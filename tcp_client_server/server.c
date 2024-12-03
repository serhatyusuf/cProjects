#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define IP "0.0.0.0"
#define PORT 8181


int main(){
    int s,c;
    socklen_t addrlen;
    //struct sockaddr_in: IP adresleri ve port numaralarını tutmak için kullanılan yapı.
    struct sockaddr_in srv,cli;
    char buf[512];
    char *data;
    int n;

    /*
    socket(): TCP/IP protokolüne uygun bir soket oluşturur.
    AF_INET: IPv4 adres ailesi.
    SOCK_STREAM: TCP tipi soket.
    Dönüş değeri: Başarılıysa soket tanımlayıcısı, başarısızsa -1.
    */
    memset(&srv,0,sizeof(srv));
    memset(&cli,0,sizeof(cli));
    s = socket(AF_INET,SOCK_STREAM,0);
    if(s<0){
        printf("socket() error\n");
        return -1;
    }

    //htons(): Port numarasını doğru biçimde ağ bayt sırasına çevirir.
    //sin_addr.s_addr = 0: Sunucu, tüm yerel IP adreslerinden gelen bağlantıları kabul eder.
    srv.sin_family=AF_INET;
    srv.sin_addr.s_addr=inet_addr(IP);
    srv.sin_port=htons(PORT);

    // bind(): Soketi belirli bir IP adresi ve porta bağlar.
    // Başarısızsa -1 döner ve hata mesajı yazılır.
    if(bind(s,(struct sockaddr *)&srv,sizeof(srv))){
        printf("bind() error\n");
        close(s);
        return -1;
    }
    //listen(): Sunucu, gelen bağlantıları kabul etmek için dinleme moduna geçer.
    //İkinci parametre: Bağlantı kuyruğundaki maksimum bekleme kapasitesi (5 bağlantı).
    if(listen(s,5)){
        printf("listen() error\n");
        close(s);
        return -1;
    }

    printf("Listening on %s:%d\n",IP,PORT);

    //accept(): Gelen bir bağlantıyı kabul eder ve yeni bir soket (c) döner.
    //Başarısızsa -1 döner. Bu noktada sunucu, istemci ile iletişim kurabilir.
    c = accept(s,(struct sockaddr *)&srv,&addrlen);
    if(c<0){
        printf("accept() error\n");
        close(s);
        return -1;
    }

    //read(): İstemciden gelen veriyi okur ve buf tamponuna yazar.
    //write(): İstemciye bir metin (HTTPD V1.0\n) gönderir.
    //Gönderilen yanıt, basit bir metin mesajıdır ve bir HTTP sunucusu gibi davranır.
    n = read(c,buf,511);
    data = "HTTPD V1.0\n";
    write(c,data,strlen(data));
    write(1,buf,n);
    close(c);
    close(s);

    return 0;
}