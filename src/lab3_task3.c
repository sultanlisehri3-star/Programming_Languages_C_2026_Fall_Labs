/*
 * Lab 3 - Task 3
 * Name: YOUR NAME
 * Student ID: YOUR STUDENT ID
 */

#include <stdio.h>

int my_strlen(const char *str);
void my_strcpy(char *dest, const char *src);

int my_strlen(const char *str)
{
    int length = 0;

    while (str[length] != '\0')
    {
        length++;
    }

    return length;
}

void my_strcpy(char *dest, const char *src)
{
    int i = 0;

    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
}

int main(void)
{
    char text[] = "Programming in C";
    char buffer[100];

    printf("Length: %d\n", my_strlen(text));

    my_strcpy(buffer, text);

    printf("Copy: %s\n", buffer);

    return 0;
}