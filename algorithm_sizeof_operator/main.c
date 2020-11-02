#include <stdio.h>
#include <stdlib.h>

int main()
{
    unsigned char a = 1;
    unsigned int count_bits = 0;
    while(a != 0)
    {
        a = a << 1;
        printf("%d\n", a);
        count_bits++;
    }
    printf("%d\n", count_bits);
    return 0;
}
