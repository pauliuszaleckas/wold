#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>

static void listen_wol(uint16_t port)
{
	int sfd;
	struct sockaddr_in si;
	char buf[256];

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1) {
		perror("Failed to open socket");
		exit(errno);
	}

	memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	si.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (struct sockaddr *)&si, sizeof(si)) == -1) {
		perror("Failed open port");
		exit(errno);
	}

	while (1) {
		int len;

		len = recvfrom(sfd, buf, sizeof(buf), 0, NULL, NULL);
		if (len == -1) {
			perror("Error while reading socket");
			exit(errno);
		}

		printf("Received data, len=%u\n", len);
	}
}

static void usage(char *app)
{
	fprintf(stderr, "Usage: %s [-p port] [-f]\n", app);
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
				usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (!foreground)
		daemon(0, 0);

	listen_wol((uint16_t)port);

	return 0;
}
