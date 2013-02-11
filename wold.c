#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>

#define BUFLEN 256

int main(int argc, char *argv[])
{
	int sfd;
	struct sockaddr_in si;
	char buf[BUFLEN];

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd == -1) {
		perror("Failed to open socket");
		return errno;
	}

	memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_port = htons(9);
	si.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sfd, (struct sockaddr *)&si, sizeof(si)) == -1) {
		perror("Failed open port");
		return errno;
	}

	while (1) {
		int len;

		len = recvfrom(sfd, buf, BUFLEN, 0, NULL, NULL);
		if (len == -1) {
			perror("Error while reading socket");
			return errno;
		}

		printf("Received data, len=%u\n", len);
	}

	return 0;
}
