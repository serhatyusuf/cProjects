#include <stdio.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#define IP "172.217.169.206"
#define PORT 80
//HTTP sunucusuna HEAD isteği gönderir ve gelen cevabı okur. HEAD, yalnızca HTTP başlıklarını döndürür
int main(){
    int s;
    struct sockaddr_in sock;

    char buf[512];
    char *data;

    data = "HEAD / HTTP/1.0\r\n\r\n";

    s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0){
        printf("socket error");
        return -1;
    }

    sock.sin_addr.s_addr=inet_addr(IP);
    sock.sin_port=htons(PORT);
    sock.sin_family=AF_INET;
    if(connect(s, (struct sockaddr *)&sock, sizeof(sock)) != 0 ){
    //if(connect(s,(struct sockaddr *)&sock,sizeof(struct sockaddr)) != 0 ){
        printf( "connection() error\n" );
        close(s);
        return -1;
    }

    write(s,data,strlen(data));

    //read() çağrısı tam yanıtı almayabilir, çünkü TCP verileri bölerek gönderebilir. Büyük bir yanıt bekliyorsanız döngüyle read() yapmanız gerekir.
    int bytes_read = read(s,buf,511);
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';  // String sonlandırıcı
    }
    close(s); 
    printf("\n%s\n",buf);
    return 0;
}
