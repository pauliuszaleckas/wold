#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

static void err_handler(char *err)
{
	syslog(LOG_ERR, "%s: %m", err);
	exit(errno);
}

static void listen_wol(uint16_t port)
{
	int sfd;
	struct sockaddr_in si;
	char buf[256];

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1)
		err_handler("Failed to open socket");

	memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (struct sockaddr *)&si, sizeof(si)) == -1)
		err_handler("Failed open port");

	while (1) {
		int len;

		len = recvfrom(sfd, buf, sizeof(buf), 0, NULL, NULL);
		if (len == -1)
			err_handler("Error while reading socket");

		syslog(LOG_INFO, "Received data, len=%u\n", len);
	}
}

static void usage()
{
	fprintf(stderr, "Usage: wold [-p port] [-f]\n");
	fprintf(stderr, "\t-p\tport to listen for WOL packet (default: 9)\n");
	fprintf(stderr, "\t-f\tstay in foreground\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int foreground = 0;
	unsigned long int port = 9;

	while ((opt = getopt(argc, argv, "p:f")) != -1) {
		switch (opt) {
			case 'p':
				port = strtoul(optarg, NULL, 0);
				if (port > USHRT_MAX) {
					perror("Invalid port");
					exit(errno);
				}
				break;
			case 'f':
				foreground = 1;
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

	if (!foreground) {
		if (daemon(0, 0) < 0) {
			perror("Failed to daemonize");
			exit(errno);
		}

		openlog("wold", LOG_PID, LOG_DAEMON);
	} else
		openlog("wold", LOG_PERROR | LOG_PID, LOG_DAEMON);

	listen_wol((uint16_t)port);

	return 0;
}
