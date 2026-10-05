/*
 * week4_1_dynamic_array.c
 * Author: Sehri Sultanli
 * Student ID: 251ADB093
 * Description:
 *   Demonstrates creation and usage of a dynamic array using malloc.
 *   Allocates memory for n integers, reads them from the user,
 *   prints their sum and average, and then frees the memory.
 */

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int n;
    int *arr = NULL;
    int sum = 0;
    double average;

    printf("Enter number of elements: ");

    if (scanf("%d", &n) != 1 || n <= 0)
    {
        printf("Invalid size.\n");
        return 1;
    }

    /* Allocate memory for n integers. */
    arr = malloc(n * sizeof(int));

    /* Check that memory allocation was successful. */
    if (arr == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("Enter %d integers: ", n);

    /* Read the integers into the dynamically allocated array. */
    for (int i = 0; i < n; i++)
    {
        if (scanf("%d", &arr[i]) != 1)
        {
            printf("Invalid input.\n");
            free(arr);
            return 1;
        }
    }

    /* Calculate the sum. */
    for (int i = 0; i < n; i++)
    {
        sum += arr[i];
    }

    /* Calculate the average using floating-point division. */
    average = (double)sum / n;

    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", average);

    /* Release dynamically allocated memory. */
    free(arr);

    return 0;
}