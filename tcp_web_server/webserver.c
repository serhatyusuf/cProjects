#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define LISTENADDR "0.0.0.0"
typedef struct 
{
    char method[8];
    char url[128];
}httpreq;

typedef struct {
    char filename[64];
    char *fc;
    int size;
}File;

/* return 0 on error ,or it returns a socket fd*/
int srv_init(int portno){
    //Bu, socket dosya tanımlayıcısını (file descriptor) tutacak değişken.
    //Socket işlemleri üzerinde işlem yapmak için kullanılır.
    int s;
   
    //Sunucu adres bilgilerini tutmak için kullanılan bir struct tanımı.
    //Bu yapı, TCP/IP protokolü için bir soketin IP adresi ve port numarasını içerir.
    struct sockaddr_in srv;

    
    s=socket(AF_INET,SOCK_STREAM,0);
    /*
        socket fonksiyonu, bir socket oluşturur ve tanımlayıcısını döner.
        AF_INET: IPv4 protokol ailesini belirtir.
        SOCK_STREAM: TCP protokolünü (bağlantı tabanlı iletişim) seçer.
        0: Varsayılan protokol (TCP için IPPROTO_TCP).

        Sonuç: Eğer socket başarıyla oluşturulursa, s pozitif bir dosya tanımlayıcı 
        döner. Aksi halde hata oluşmuşsa s negatif olur.
    */
    if(s<0)
        return 0;
    //Eğer socket() negatif bir değer dönerse (örneğin, sistem kaynağı tükenirse), 
    //fonksiyon 0 döner ve sunucu başlatılamaz.
    srv.sin_family = AF_INET;
    //srv yapısının sin_family alanı, soketin hangi adres ailesine ait 
    //olduğunu belirtir. Burada IPv4 ailesi belirtiliyor.
    srv.sin_addr.s_addr=inet_addr(LISTENADDR);
    /*
    sin_addr.s_addr alanı, sunucunun dinleyeceği IP adresini belirtir.

    inet_addr(LISTENADDR): LISTENADDR adındaki IP adresini ağ formatına çevirir.
        Örneğin, "127.0.0.1" (loopback adresi) verilebilir.
    Eğer sunucu herhangi bir IP adresinde dinleyecekse, INADDR_ANY kullanılabilir 
    (genelde 0.0.0.0 anlamına gelir).
    */
    srv.sin_port = htons(portno);
    /*
    sin_port alanı, ağ üzerinde kullanılacak bağlantı noktasını (port) belirtir.

        htons(portno): Port numarasını, host'tan network byte sıralamasına çevirir.
        Neden? Çünkü ağ protokolü, veriyi "big-endian" formatında kullanır.
    */
    if(bind(s,(struct sockaddr *)&srv,sizeof(srv))){
        /*
        bind() fonksiyonu, socket'i belirtilen IP adresine ve port numarasına bağlar.

        s: Socket tanımlayıcı.
        (struct sockaddr *)&srv: Socket adres bilgisi (genel sockaddr yapısına 
        dönüştürülür). sizeof(srv): Adres yapısının boyutu.
        */
        close(s);
        //Socket başarıyla bağlanamazsa (örneğin, bind başarısız olduysa),
        // açık olan socket kapatılır.
        return 0;
    }

    if(listen(s,5)){
        // listen() fonksiyonu, socket'i gelen bağlantılar için dinleme moduna geçirir.
        // 5: İzin verilen bekleme kuyruğu uzunluğunu belirtir. 
        // Bu, aynı anda kaç bağlantının bekleyebileceğini kontrol eder.
        close(s);
        return 0;
    }

    return s;
}

int cli_accept(int s){
    // c, kabul edilen istemci bağlantısını temsil edecek olan socket tanımlayıcısını tutar.
    // Bu da işletim sistemi tarafından atanır ve gelen istemci bağlantısı için benzersizdir.
    int c;
    // addrlen, istemci adres yapısının boyutunu tutar.
    // Bu, accept fonksiyonu tarafından güncellenerek istemcinin adres bilgilerinin uzunluğunu belirler.
    socklen_t addrlen;
    addrlen = 0;
    //cli, istemci adres bilgilerini tutmak için kullanılan bir yapı.
    //Bu yapı, istemcinin IP adresi ve bağlantı portu gibi bilgileri içerir.
    struct sockaddr_in cli;
    //cli yapısındaki tüm verileri sıfırlar (0'a ayarlar).
    //Bu, yapının başlangıçta "temiz" olmasını ve eski veri kalıntılarının olmamasını sağlar.
    memset(&cli,0,sizeof(cli));
    /*
        accept, sunucunun bir istemci bağlantısını kabul etmesini sağlar.
    Parametreler:
        s: Dinleme modundaki sunucu socket tanımlayıcısı.
        (struct sockaddr *)&cli: Kabul edilen istemcinin adres bilgilerini depolamak için kullanılan bir yapı.
        &addrlen: cli yapısının boyutunu içerir ve accept tarafından güncellenir.

    Çalışma Mantığı:

        Eğer bir istemci sunucuya bağlanmak istiyorsa, accept bu bağlantıyı kabul eder ve:
            Yeni bir socket dosya tanımlayıcısı döner (c).
            İstemci adres bilgilerini cli yapısına yazar.

    Sonuç:

        Eğer bağlantı kabul edilirse, yeni bir istemci socket tanımlayıcısı döner (pozitif bir sayı).
        Eğer hata oluşursa, c negatif bir değer alır ve bağlantı kabul edilemez.
    */
    c=accept(s,(struct sockaddr *)&cli,&addrlen);
    if(c<0){
        //Eğer accept negatif bir değer dönerse, bir hata oluşmuş demektir (örneğin, bağlantı isteği sırasında bir kesinti olmuş olabilir).
        //Bu durumda: Fonksiyon 0 döner ve istemci bağlantısı kabul edilmez.
        return 0;
    }
    return c;
}

//returns 0 is error , or returns a httpreq struct
httpreq *parse_http(char *str){
    httpreq *req;
    req=malloc(sizeof(httpreq));
    memset(req,0,sizeof(httpreq));
   char *p;

    // İlk boşluğu bul
    p = strchr(str, ' ');
    if (!p) {
        printf("parse_http error: Invalid input string\n");
        free(req);
        return 0;
    }
    *p = '\0'; // Boşluğu sonlandırıcı karakter yap
    strncpy(req->method, str, sizeof(req->method) - 1);
    req->method[sizeof(req->method) - 1] = '\0'; // Sonlandırıcı ekle

    // Yeni başlangıç noktası
    str = p + 1;

    // İkinci boşluğu bul
    p = strchr(str, ' ');
    if (!p) {
        printf("parse_http error: Missing URL or HTTP version\n");
        free(req);
        return 0;
    }
    *p = '\0'; // Boşluğu sonlandırıcı karakter yap
    strncpy(req->url, str, sizeof(req->url) - 1);
    req->url[sizeof(req->url) - 1] = '\0'; // Sonlandırıcı ekle
    
    return req;
} 

char *cli_read(int c){
    static char buf[512];
    memset(buf,0,512);
    if(read(c,buf,511)<0){
        printf("error in cli_read function\n");
        return 0;
    }else{
        return buf;
    }
}

void http_respose(int c,char *contenttype,char *data){
    char buf[512];
    int n;
    
    n=strlen(data);
    memset(buf,0,512);
    snprintf(buf,511,
    "Content-Type: %s\n"
    "Content-Length: %d\n"
    "\n%s\n",contenttype,n,data);

    n=strlen(buf);
    write(c,buf,n);
 
}

void http_headers(int c,int code){
    char buf[512];
    int n;
    memset(buf,0,511);
    snprintf(buf, 511,
    "HTTP/1.0 %d OK\n"
    "Server: httpd.c\n"
    "Cache-Control: no-store, no-cache, max-age=0, private\n"
    "Content-Language: en\n"
    "Expires: -1\n"
    "X-Frame-Options: SAMEORIGIN\n",
    code);
    n= strlen(buf);
    write(c,buf,n);
}

void cleanup(File *f, int fd) {
    if (fd >= 0) close(fd);
    if (f) {
        free(f->fc);
        free(f);
    }
}
/* return o for error , or return struct*/
File *readfile(char *filename){
    char buf[512];
    //char *p;
    int total_read,fd;
    File *f = NULL;
    total_read = fd = 0;

    fd= open(filename,O_RDONLY);
    if(fd<0){
        return 0;
    }

    f = malloc(sizeof(File));
    if(!f){
        close(fd);
        return 0;
    }
    //Dosya adı f yapısının filename alanına kopyalanıyor. En fazla 63 karakter kopyalanıyor, bu tür güvenlik önlemleri taşma riskini azaltır.
    strncpy(f->filename,filename,63);
    //Dosya içeriği için 512 baytlık bir başlangıç tamponu ayrılıyor. f->fc, dosya içeriğini tutacak bir işaretçi.
    f->fc = malloc(512);
     if (!f->fc) {
        cleanup(f,fd);
        return 0;
    }

    while(1){
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0) {
            cleanup(f, fd);
            return 0; 
        }

        if (n == 0) break;

        memcpy((f->fc)+total_read, buf, n);
        total_read +=n;
        f->fc = realloc(f->fc,(512+total_read));
        if (!f->fc) {
            cleanup(f, fd);
            return NULL;
        }
    }
    f->size = total_read;
    close(fd);
    return f;
}
/*return 1 succecs , 0 error*/
int sendfile(int c,char *contenttype,File *file){
    if(!file)
        return 0;
    
    char buf[512];
    char *p;
    
    memset(buf,0,512);
    
    int n , x;
    n = x = 0;

    snprintf(buf,511,
    "Contetn-Type:%s\n"
    "Content-Length: %d\n\n",
    contenttype,file->size);

    n = strlen(buf);
    write(c, buf, n);

    n = file->size;
    p = file->fc;
    

    while(1){
        x = write(c,p,(n<512)?n:512);
        if(x<1)
            return 0;
        n -= x;
        if(n<1)
            break;
        else
            p += x; 
    }

    return 1;
}

void cli_conn(int s, int c){
    httpreq *req;
    char str[96];
    char *p;
    char *res;
    File *f;

    p=cli_read(c);
    if(!p){
        printf("cli_read error\n");
        close(c);
        return;
    }

    req = parse_http(p);
    if(!req){
        printf("http_parse error \n");
        close(c);
        return;
    }
    if(!strcmp(req->method,"GET") && !strncmp(req->url,"/img/",5)){
        
        if(strstr(req->url , "..")){
            http_headers(c,300);
            res="access denied";
            http_respose(c,"text/plain",res);
        }
        
        memset(str,0,96);
        snprintf(str,95,".%s",req->url);
        f=readfile(str);
        if(!f){
            res="<html>File not found 404</html>";
            http_headers(c,404);
            http_respose(c,"text/html",res);
        }else{
            http_headers(c,200);
            if(!sendfile(c,"image/png",f)){
                res="<html>HTML Server Error 500</html>";
                //http_headers(c,500);
                http_respose(c,"text/plain",res);
            }
        }
    }
    else if(!strcmp(req->method,"GET") && !strcmp(req->url,"/app/")){
        res="<html><h4>Hi there!!!</h4><br><img src='/img/test.png' width='200px'/></html>";
        http_headers(c,200);
        http_respose(c,"text/html",res);
    }else{
        res="<html>File not found 404</html>";
        http_headers(c,404);
        http_respose(c,"text/html",res);

    }

    free(req);
    close(c);
}

int main(int argc , char *argv[]){
    int s,c;
    char *port;
    
    if(argc < 2){
        fprintf(stderr,"Usage : %s <listening port>\n",argv[0]);
        return -1;
    }else{
        port=argv[1];
    }

    s = srv_init(atoi(port));
    if(!s){
        fprintf(stderr,"%s\n","srv_init hata");
        return -1;
    }

    printf("Listening on %s:%d\n",LISTENADDR,atoi(port));
    while (1)
    {
        c=cli_accept(s);
        if(!c){
            fprintf(stderr,"%s\n","cli_accept hata");
            continue;
        }
        printf("incoming connection\n");
        if(!fork())
            cli_conn(s,c);
        
        //sleep(1);
    }
    return -1;
}