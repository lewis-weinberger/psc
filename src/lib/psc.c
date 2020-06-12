#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#include "psc.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define PLACE __FILE__ ":" TOSTRING(__LINE__)

typedef struct
{
	void            *game;
	size_t          len;
	pthread_mutex_t mutex;
} psc_state;

typedef struct
{
	int             exit; /* bad */
	int             quit; /* good */
	int             id;
	pthread_mutex_t mutex;
} psc_status;

static psc_state state;
static FILE *log;
static psc_status status;
static pthread_t *tid;
static int *cfd, sfd;
static struct sockaddr_un addr;
const  char socket_path[] = "/tmp/psc_socket";

static int  cfroms(void*, size_t);
static void client_main(psc_init, psc_loop, psc_fin);
static void *client_update(void*);
static int  ctos(void*, size_t);
static void *emalloc(size_t);
static void log_error(const char*, const char*);
static void server_main(psc_init, psc_loop, psc_fin, int);
static void *server_update(void*);
//static void *signal_handler(void*);
//static void signal_setup(void);
static int  sfromc(int, void*, size_t);
static void socket_connect(void);
static void socket_init(int);
static void socket_fin(void);
static int  stoc(int, void*, size_t);

/*
 * Run game loop.
 * game: pointer to game state.
 * len: size of game state in bytes.
 * init: function to execute prior to starting game loop.
 * loop: function to execute at each game loop.
 * fin: function to execute after finish of game loop.
 * nclient: number of clients (should be > 0 for server caller).
 */
void psc_run(void *game, size_t len, psc_init init, psc_loop loop,  psc_fin fin, int nclient)
{
	if((log = fopen("psc.log", "a")) == NULL)
		log = stderr;

	if (game == NULL || len <= 0 || init == NULL || loop == NULL || fin == NULL || nclient < 0)
	{
		log_error("incorrect call to psc_run", PLACE);
		fclose(log);
		return;
	}

	state.game = game;
	state.len = len;
	pthread_mutex_init(&state.mutex, NULL);

	status.exit = FALSE;
	status.quit = FALSE;
	status.id = -1;
	pthread_mutex_init(&status.mutex, NULL);

	if (nclient > 0)
	{
		tid = emalloc(nclient * sizeof(pthread_t));
		cfd = emalloc(nclient * sizeof(int));
		memset(cfd, 0, nclient * sizeof(int));
		server_main(init, loop, fin, nclient);
		free(tid);
		free(cfd);
	}
	else if (nclient == 0)
		client_main(init, loop, fin);
	else
		log_error("number of clients must be >= 0", PLACE);

	pthread_mutex_destroy(&state.mutex);
	pthread_mutex_destroy(&status.mutex);
	fclose(log);
}

static void log_error(const char *str, const char *place)
{
	time_t current_time;
	struct tm *time_info;
	char strtime[9];

	time(&current_time);
	time_info = localtime(&current_time);
	strftime(strtime, sizeof(strtime), "%H:%M:%S", time_info);

	/* Apparently POSIX guarantees fprintf is thread-safe */
	if (place != NULL)
		fprintf(log, "[%s]: %s (%s)\n", strtime, str, place);
	else
		fprintf(log, "[%s]: %s\n", strtime, str);
	fflush(log);
}

static void error_handler(const char *str, const char* place)
{
	log_error(str, place);
	pthread_mutex_lock(&status.mutex);
	status.exit = TRUE;
	pthread_mutex_unlock(&status.mutex);
}

static void server_main(psc_init init, psc_loop loop, psc_fin fin, int nclient)
{
	int i, ret, state_changed, exit, dead, quit;
	void *previous;

	/* User initialisation */
	if (init() < 0)
	{
		log_error("user init failed", PLACE);
		return;
	}

	/* Signal handling: TODO */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGWINCH, SIG_IGN);

	/* Connect to clients */
	socket_init(nclient);
	for (i = 0; i < nclient; i++)
	{
		if ((cfd[i] = accept(sfd, NULL, NULL)) < 0)
			log_error("accept error", PLACE);
		else
		{
			stoc(cfd[i], &i, sizeof(i));
			log_error("* Client connected", NULL);
			pthread_create(&tid[i], NULL, &server_update, &cfd[i]);
		}
	}

	previous = emalloc(state.len);
	memcpy(previous, state.game, state.len);

	/* Run game loop */
	for (;;)
	{
		/* Check for exit from other threads */
		pthread_mutex_lock(&status.mutex);
		exit = status.exit;
		dead = status.id;
		pthread_mutex_unlock(&status.mutex);
		if (exit)
		{
			for (i = 0; i < nclient; i++)
				pthread_join(tid[i], NULL);
			break;
		}

		/* Check for disconnection from other clients */
		for (i = 0; i < nclient; i++)
		{
			if (cfd[i] == dead)
			{
				pthread_join(tid[i], NULL);
				close(dead);

				/* Reconnect and spawn new thread */
				if ((cfd[i] = accept(sfd, NULL, NULL)) < 0)
					log_error("accept error", PLACE);
				else
				{
					stoc(cfd[i], &i, sizeof(i));
					log_error("* Client reconnected", NULL);
					pthread_create(&tid[i], NULL, &server_update, &cfd[i]);

					pthread_mutex_lock(&status.mutex);
					status.id = dead = -1;
					pthread_mutex_unlock(&status.mutex);
				}
			}
		}

		/* Evaluate user defined procedure */
		pthread_mutex_lock(&state.mutex);
		state_changed = FALSE;
		if (memcmp(previous, state.game, state.len) != 0)
		{
			state_changed = TRUE;
			memcpy(previous, state.game, state.len);
		}
		ret = loop(state.game, state_changed, nclient);
		pthread_mutex_unlock(&state.mutex);

		/* User quit? */
		pthread_mutex_lock(&status.mutex);
		if (ret < 0)
			status.quit = TRUE;
		quit = status.quit;
		pthread_mutex_unlock(&status.mutex);
		if (quit)
		{
			for (i = 0; i < nclient; i++)
				pthread_join(tid[i], NULL);
			break;
		}

		/* TODO: tick time? */
	}

	/* Clean-up */
	free(previous);
	socket_fin();
	fin();
}

static void client_main(psc_init init, psc_loop loop, psc_fin fin)
{
	int ret, state_changed, player_id, exit, quit;
	pthread_t thread;
	void *previous;

	/* User initialisation */
	if (init() < 0)
	{
		log_error("user init failed", PLACE);
		return;
	}

	/* Signal handling: TODO */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGWINCH, SIG_IGN);

	/* Connect to server */
	socket_connect();
	cfroms(&player_id, sizeof(player_id));
	log_error("* Connected to server", NULL);
	pthread_create(&thread, NULL, &client_update, NULL);

	previous = emalloc(state.len);
	memcpy(previous, state.game, state.len);

	/* Run game loop */
	for (;;)
	{
		/* Check for exit from other thread */
		pthread_mutex_lock(&status.mutex);
		exit = status.exit;
		pthread_mutex_unlock(&status.mutex);
		if (exit)
		{
			pthread_join(thread, NULL);
			break;
		}

		/* Evaluate user defined procedure */
		pthread_mutex_lock(&state.mutex);
		state_changed = FALSE;
		if (memcmp(previous, state.game, state.len) != 0)
		{
			state_changed = TRUE;
			memcpy(previous, state.game, state.len);
		}
		ret = loop(state.game, state_changed, player_id);
		pthread_mutex_unlock(&state.mutex);

		/* User quit? */
		pthread_mutex_lock(&status.mutex);
		if (ret < 0)
			status.quit = TRUE;
		quit = status.quit;
		pthread_mutex_unlock(&status.mutex);
		if (quit)
		{
			pthread_join(thread, NULL);
			break;
		}


		/* TODO: tick time? */
	}

	/* Clean-up */
	free(previous);
	close(sfd);
	log_error("* Client closed", NULL);
	fin();
}

static void socket_init(int nclient)
{
	/* Create socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		log_error("socket error", PLACE);
		exit(-1);
	}

	/* Bind socket to path */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
	unlink(socket_path);
	if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		log_error("bind error", PLACE);
		close(sfd);
		unlink(socket_path);
		exit(-1);
	}
	log_error("* Listening on socket", socket_path);

	/* Ensure permissions on socket allow (any) other users to join */
	if(chmod(socket_path, S_IRWXG | S_IRWXU | S_IRWXO) < 0)
		log_error("chmod error", PLACE);

	/* Specify maximum number of connections */
	if (listen(sfd, nclient) < 0)
	{
		log_error("listen error", PLACE);
		close(sfd);
		unlink(socket_path);
		exit(-1);
	}
}

static void socket_connect(void)
{
	/* Open socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		log_error("socket error", PLACE);
		close(sfd);
		exit(-1);
	}

	/* Connect to server */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
	if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		log_error("connect error", PLACE);
		close(sfd);
		exit(-1);
	}
}

static void socket_fin(void)
{
	close(sfd);
	unlink(socket_path);
	log_error("* Server closed", NULL);
}

static void *server_update(void *arg)
{
	int alt, client, exit, id, quit;
	void *previous;

	client = *(int *)arg;

	pthread_mutex_lock(&state.mutex);
	previous = emalloc(state.len);
	memcpy(previous, state.game, state.len);
	stoc(client, state.game, state.len);
	pthread_mutex_unlock(&state.mutex);

	for (;;)
	{
		/* Check for exit or quit from other threads */
		pthread_mutex_lock(&status.mutex);
		exit = status.exit;
		id = status.id;
		quit = status.quit;
		pthread_mutex_unlock(&status.mutex);
		stoc(client, &quit, sizeof(quit));
		if (!quit)
		{
			sfromc(client, &quit, sizeof(quit));
			pthread_mutex_lock(&status.mutex);
			status.quit = quit;
			pthread_mutex_unlock(&status.mutex);
		}

		if (quit || exit || id == client)
			break;

		pthread_mutex_lock(&state.mutex);

		/* Send any changes to client */
		alt = memcmp(previous, state.game, state.len);
		stoc(client, &alt, sizeof(int));
		if (alt != 0)
		{
			stoc(client, state.game, state.len);
			memcpy(previous, state.game, state.len);
		}

		/* Receive any changes from client */
		sfromc(client, &alt, sizeof(int));
		if (alt != 0)
		{
			sfromc(client, state.game, state.len);
			memcpy(previous, state.game, state.len);
		}

		pthread_mutex_unlock(&state.mutex);
	}

	free(previous);
	return 0;
}

static void *client_update(void *arg)
{
	int alt, exit, quit, sendquit;
	void *previous;

	pthread_mutex_lock(&state.mutex);
	previous = emalloc(state.len);
	cfroms(state.game, state.len);
	memcpy(previous, state.game, state.len);
	pthread_mutex_unlock(&state.mutex);

	for (;;)
	{
		/* Check for quit */
		cfroms(&quit, sizeof(quit));
		sendquit = !quit;

		/* Check for exit from other threads */
		pthread_mutex_lock(&status.mutex);
		exit = status.exit;
		if (quit)
			status.quit = quit;
		else
			quit = status.quit;
		pthread_mutex_unlock(&status.mutex);
		if (sendquit)
			ctos(&quit, sizeof(quit));
		if (quit || exit)
			break;

		pthread_mutex_lock(&state.mutex);

		/* Receive any changes from server */
		cfroms(&alt, sizeof(int));
		if (alt != 0)
		{
			cfroms(state.game, state.len);
			memcpy(previous, state.game, state.len);
		}

		/* Send any changes to server */
		alt = memcmp(previous, state.game, state.len);
		ctos(&alt, sizeof(int));
		if (alt != 0)
		{
			ctos(state.game, state.len);
			memcpy(previous, state.game, state.len);
		}

		pthread_mutex_unlock(&state.mutex);
	}

	free(previous);
	return 0;
}

static int sfromc(int client, void *msg, size_t len)
{
	int n;

	if ((n = read(client, msg, len)) < 0)
	{
		if (errno == EINTR) /* Interrupted... try again */
			sfromc(client, msg, len);
		else if (errno == ECONNRESET) /* Client disconnected */
		{
			log_error("* Client disconnect", strerror(errno));
			pthread_mutex_lock(&status.mutex);
			status.id = client;
			pthread_mutex_unlock(&status.mutex);
		}
		else
			error_handler("read error", strerror(errno));
	}

	if (n == 0) /* Client disconnected */
	{
		pthread_mutex_lock(&status.mutex);
		status.id = client;
		pthread_mutex_unlock(&status.mutex);
		log_error("* Client disconnect", "read");
	}
	return n;
}

static int stoc(int client, void *msg, size_t len)
{
	int n;

	if ((n = write(client, msg, len)) < 0)
	{
		if (errno == EINTR) /* Interrupted... try again */
			stoc(client, msg, len);
		else if (errno == EPIPE) /* Client disconnected */
		{
			pthread_mutex_lock(&status.mutex);
			status.id = client;
			pthread_mutex_unlock(&status.mutex);
			log_error("* Client disconnect", "write");
		}
		else
			error_handler("write error", strerror(errno));
	}
	return n;
}

static int cfroms(void *msg, size_t len)
{
	int n;

	if ((n = read(sfd, msg, len)) <= 0)
	{
		if (errno == EINTR) /* Interrupted... try again */
			cfroms(msg, len);
		else
			error_handler("read error", strerror(errno));
	}
	return n;
}

static int ctos(void *msg, size_t len)
{
	int n;

	if ((n = write(sfd, msg, len)) < 0)
	{
		if (errno == EINTR) /* Interrupted... try again */
			ctos(msg, len);
		else
			error_handler("write error", strerror(errno));
	}
	return n;
}

static void *emalloc(size_t len)
{
	void *p;

	if ((p = malloc(len)) == NULL)
	{
		log_error("malloc error", PLACE);
		exit(-1);
	}
	return p;
}
