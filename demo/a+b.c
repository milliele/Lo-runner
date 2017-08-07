#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    int a, b;
    
    while(~scanf("%d %d", &a, &b)){
        printf("%d\n", a * b);
    }
//    printf("2\n4\n6\n8\n10\n");
    // open("asdf", O_CREAT);
    /*scanf("%d", &a);*/
    
    return 0;
}
