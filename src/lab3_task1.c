/*
 * Lab 3 - Task 1
 * Name: YOUR NAME
 * Student ID: YOUR STUDENT ID
 */

#include <stdio.h>

int array_min(int arr[], int size);
int array_max(int arr[], int size);
int array_sum(int arr[], int size);
float array_avg(int arr[], int size);

int array_min(int arr[], int size)
{
    int min = arr[0];

    for (int i = 1; i < size; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }

    return min;
}

int array_max(int arr[], int size)
{
    int max = arr[0];

    for (int i = 1; i < size; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }

    return max;
}

int array_sum(int arr[], int size)
{
    int sum = 0;

    for (int i = 0; i < size; i++)
    {
        sum += arr[i];
    }

    return sum;
}

float array_avg(int arr[], int size)
{
    return (float)array_sum(arr, size) / size;
}

int main(void)
{
    int arr[] = {10, 20, 5, 30, 15};

    printf("Min: %d\n", array_min(arr, 5));
    printf("Max: %d\n", array_max(arr, 5));
    printf("Sum: %d\n", array_sum(arr, 5));
    printf("Avg: %.2f\n", array_avg(arr, 5));

    return 0;
}