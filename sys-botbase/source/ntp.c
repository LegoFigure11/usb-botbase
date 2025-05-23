/* This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.
In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
For more information, please refer to <https://unlicense.org>
Additionally, the ntp_packet struct uses code licensed under the BSD 3-clause. See LICENSE-THIRD-PARTY for more
information. */

#define _BSD_SOURCE

#include "ntp.h"
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <switch.h>
#include <sys/socket.h>
#include <unistd.h>

time_t ntpGetTime() {
    static const char* SERVER_NAME = "0.pool.ntp.org";

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        return 0;

    struct hostent* server;
    server = gethostbyname(SERVER_NAME);
    if (!server) {
        close(sockfd);
        return 0;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;

    // Validate server->h_addr_list[0] before using it
    if (server->h_addr_list == NULL || server->h_addr_list[0] == NULL) {
        close(sockfd);
        return 0;
    }

    // Ensure the address length is sufficient
    if (server->h_length < sizeof(serv_addr.sin_addr.s_addr)) {
        close(sockfd);
        return 0;
    }

    memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr_list[0], sizeof(serv_addr.sin_addr.s_addr));
    serv_addr.sin_port = htons(123);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return 0;
    }

    ntp_packet packet;
    memset(&packet, 0, sizeof(ntp_packet));
    packet.li_vn_mode = (0 << 6) | (4 << 3) | 3;              // LI 0 | Client version 4 | Mode 3
    packet.txTm_s = htonl(NTP_TIMESTAMP_DELTA + time(NULL));  // Current networktime on the console

    if (send(sockfd, (char*)&packet, sizeof(ntp_packet), 0) <= 0) { // Return value is bytes received
        close(sockfd);
        return 0;
    }
    if (recv(sockfd, (char*)&packet, sizeof(ntp_packet), 0) <= 0) { // Return value is bytes received
        close(sockfd);
        return 0;
    }

    packet.txTm_s = ntohl(packet.txTm_s);
    close(sockfd);
    return (time_t)(packet.txTm_s - NTP_TIMESTAMP_DELTA);
}
