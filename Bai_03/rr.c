#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum Criteria
{
    BY_PID,
    BY_ARRIVAL,
    BY_REMAIN_BURST,
    BY_BURST,
    BY_START
};

#define UNDEFINED -1

#define MAX_PCB 10

#define MIN_ARRIVAL 0
#define MAX_ARRIVAL 20
#define MIN_BURST 2
#define MAX_BURST 12

#define MIN_QUANTUM 3
#define MAX_QUANTUM 6

typedef struct
{
    int iPID;
    int iArrival, iRemainBurst, iBurst;
    int iStart, iFinish, iWaiting, iResponse, iTaT;
} PCB;

void genProcess(int numberOfProcess, PCB *processArr[]);
void inputProcess(int numberOfProcess, PCB *processArr[]);
void printProcess(int numberOfProcess, PCB *processArr[]);
void writeLog(int *numberOfLog, int logArray[][2], int iTimePoint, PCB *);
void exportGanttChart(int numberOfLog, int logArray[][2]);
void pushProcess(int *numberOfProcess, PCB *processArr[], int index, PCB *process);
void removeProcess(int *numberOfProcess, PCB *processArr[], int index);
int compareProcess(PCB *processA, PCB *processB, int iCriteria);
void swapProcess(PCB **processA, PCB **processB);
int checkPCBStart(PCB *process);
void setPCBStart(PCB *process, int iTimePoint);
void setPCBFinish(PCB *process, int iTimePoint);
int partition(PCB *processArr[], int low, int high, int iCriteria);
void quickSort(PCB *processArr[], int low, int high, int iCriteria);
void calculateAWT(int numberOfProcess, PCB *processArr[]);
void calculateATaT(int numberOfProcess, PCB *processArr[]);

int main(int argc, char *argv[])
{
    printf("===== RR Scheduling =====\n");
    srand(time(NULL));
    // Initialize array of PCB
    PCB *InputArray[MAX_PCB];
    for (int i = 0; i < MAX_PCB; i++)
        InputArray[i] = NULL;

    PCB *ReadyQueue[MAX_PCB];
    for (int i = 0; i < MAX_PCB; i++)
        ReadyQueue[i] = NULL;

    PCB *TerArray[MAX_PCB];
    for (int i = 0; i < MAX_PCB; i++)
        TerArray[i] = NULL;

    // Initialize array of log file
    // - First column is time point
    // - Second column is PID
    int LogProcess[5 * MAX_PCB][2];

    // Input number of process
    // If no argument, input from keyboard
    // And manual input processes and TIME_QUANTUM
    // Else, generate random number of process
    // And auto generate random number of process
    int iNumberOfProcess;
    int TIME_QUANTUM;
    if (argc < 2)
    {
        printf("Please input number of Process: ");
        scanf("%d", &iNumberOfProcess);
        inputProcess(iNumberOfProcess, InputArray);
        int time_quantum;
        printf("Please input TIME_QUANTUM: ");
        scanf("%d", &TIME_QUANTUM);
    }
    else
    {
        iNumberOfProcess = atoi(argv[1]);
        genProcess(iNumberOfProcess, InputArray);
        TIME_QUANTUM = rand() % (MAX_QUANTUM - MIN_QUANTUM + 1) +
                       MIN_QUANTUM;
    }

    // Initialize number of InputArray, ReadyQueue, TerArray
    int iRemain = iNumberOfProcess, iReady = 0, iTer = 0;

    // Sort input array by arrival time
    // And print input array
    quickSort(InputArray, 0, iNumberOfProcess - 1,
              BY_ARRIVAL);
    printf("===== RR Scheduling =====\n");
    printf("\nInput Array:\n");
    printProcess(iNumberOfProcess, InputArray);

    int step = 0;
    int timePoint = 0;
    int timeLine = 0;

    // Check if any process is not terminated
    while (iTer < iNumberOfProcess)
    {
        // Check if ready queue is empty
        if (iReady == 0)
        {
            // timePoint is skipped to the early arrival time
            timePoint = InputArray[0]->iArrival;

            // Push all process that have same arrival time
            while (iRemain > 0 && InputArray[0]->iArrival <= timePoint)
            {
                pushProcess(&iReady, ReadyQueue,
                            iReady, InputArray[0]);
                removeProcess(&iRemain, InputArray, 0);
            }
        }
        // Else if ready queue is not empty
        else if (iReady > 0)
        {
            // Make sure that the current process is running
            if (!checkPCBStart(ReadyQueue[0]))
                setPCBStart(ReadyQueue[0], timePoint);

            // Increase time point by the min between remain burst time
            // And quantum time.
            if (ReadyQueue[0]->iRemainBurst <= TIME_QUANTUM)
            {
                timePoint += ReadyQueue[0]->iRemainBurst;

                // Set remain burst time to 0
                // And that process is terminated
                ReadyQueue[0]->iRemainBurst = 0;
                setPCBFinish(ReadyQueue[0], timePoint);
            }
            else
            {
                timePoint += TIME_QUANTUM;

                // Decrease remain burst time by quantum time
                ReadyQueue[0]->iRemainBurst -= TIME_QUANTUM;
            }

            // Push all process that have same arrival time
            // into ready queue
            while (iRemain > 0 && InputArray[0]->iArrival <= timePoint)
            {
                pushProcess(&iReady, ReadyQueue,
                            iReady, InputArray[0]);
                removeProcess(&iRemain, InputArray, 0);
            }

            // If current process is terminated
            if (ReadyQueue[0]->iRemainBurst == 0)
            {
                // Push current process into TerArray
                pushProcess(&iTer, TerArray,
                            iTer, ReadyQueue[0]);
                removeProcess(&iReady, ReadyQueue, 0);
            }
            // Else if current process is not terminated
            else if (ReadyQueue[0]->iRemainBurst > 0)
            {
                // Move that process to the end of ReadyQueue
                pushProcess(&iReady, ReadyQueue,
                            iReady, ReadyQueue[0]);
                removeProcess(&iReady, ReadyQueue, 0);
            }
        }

        // Write log file
        writeLog(&timeLine, LogProcess, timePoint, ReadyQueue[0]);

        // Show the step by step with the following code
        // printf("\nStep %d at time: %d\n", ++step, timePoint);

        // printf("Ready Queue:\n");
        // printProcess(iReady, ReadyQueue);

        // printf("Terminated Queue:\n");
        // printProcess(iTer, TerArray);
    }

    printf("In Result\nTerminated Queue:\n");
    printProcess(iTer, TerArray);

    printf("\n===== RR Scheduling with QUANTUM = %d =====\n", TIME_QUANTUM);

    // Export Gannt Chart
    exportGanttChart(timeLine, LogProcess);

    // Calculate AWT and ATAT
    calculateAWT(iTer, TerArray);
    calculateATaT(iTer, TerArray);

    // Free all allocated memory
    for (int i = 0; i < iRemain; i++)
        free(InputArray[i]);

    for (int i = 0; i < iReady; i++)
        free(ReadyQueue[i]);

    for (int i = 0; i < iTer; i++)
        free(TerArray[i]);

    return 0;
}

void inputProcess(int n, PCB *arr[])
{
    for (int i = 0; i < n; i++)
    {
        arr[i] = (PCB *)malloc(sizeof(PCB));
        printf("Process %d\n", i + 1);
        printf("Arrival Time: ");
        scanf("%d", &arr[i]->iArrival);
        printf("Burst Time: ");
        scanf("%d", &arr[i]->iBurst);
        arr[i]->iPID = i + 1;
        arr[i]->iRemainBurst = arr[i]->iBurst;
        arr[i]->iStart = UNDEFINED;
        arr[i]->iFinish = UNDEFINED;
        arr[i]->iWaiting = UNDEFINED;
        arr[i]->iResponse = UNDEFINED;
        arr[i]->iTaT = UNDEFINED;
    }
}

void genProcess(int n, PCB *arr[])
{
    for (int i = 0; i < n; i++)
    {
        arr[i] = (PCB *)malloc(sizeof(PCB));
        arr[i]->iPID = i + 1;
        arr[i]->iArrival = rand() % (MAX_ARRIVAL - MIN_ARRIVAL + 1) +
                           MIN_ARRIVAL;
        arr[i]->iBurst = rand() % (MAX_BURST - MIN_BURST + 1) +
                         MIN_BURST;
        arr[i]->iRemainBurst = arr[i]->iBurst;
        arr[i]->iStart = UNDEFINED;
        arr[i]->iFinish = UNDEFINED;
        arr[i]->iWaiting = UNDEFINED;
        arr[i]->iResponse = UNDEFINED;
        arr[i]->iTaT = UNDEFINED;
    }
}

void printProcess(int n, PCB *arr[])
{
    printf("------\n");
    printf("PID\tArrival\tReBurst\tBurst\tStart\tFinish\tResponse\tWaiting\tTaT\n");
    for (int i = 0; i < n; i++)
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t\t%d\t%d\n",
               arr[i]->iPID,
               arr[i]->iArrival,
               arr[i]->iRemainBurst,
               arr[i]->iBurst,
               arr[i]->iStart,
               arr[i]->iFinish,
               arr[i]->iResponse,
               arr[i]->iWaiting,
               arr[i]->iTaT);
    printf("------\n");
}

void writeLog(int *n, int arr[][2], int t, PCB *p)
{
    arr[*n][0] = t;
    arr[*n][1] = (p ? p->iPID : UNDEFINED);
    (*n)++;
}

void exportGanttChart(int n, int arr[][2])
{
    for (int i = 0; i < n; i++)
        if (arr[i][1] != arr[i - 1][1])
        {
            printf("%d ", arr[i][0]);
            printf("\t");
        }

    printf("\n| ");

    for (int i = 0; i < n - 1; i++)
        if (arr[i][1] != arr[i - 1][1])
        {
            if (arr[i][1] == UNDEFINED)
                printf("IDLE ");
            else
                printf("P%d ", arr[i][1]);
            printf("\t| ");
        }

    printf("\n");
}

void pushProcess(int *n, PCB *arr[], int ind, PCB *p)
{
    for (int i = *n; i > ind; i--)
        arr[i] = arr[i - 1];

    arr[ind] = p;
    (*n)++;
}

void removeProcess(int *n, PCB *arr[], int ind)
{
    for (int i = ind; i < *n - 1; i++)
        arr[i] = arr[i + 1];
    arr[*n - 1] = NULL;
    (*n)--;
}

int compareProcess(PCB *a, PCB *b, int c)
{
    int result = 0;
    switch (c)
    {
    case BY_PID:
        result = a->iPID - b->iPID;
        break;
    case BY_ARRIVAL:
        result = a->iArrival - b->iArrival;
        break;
    case BY_REMAIN_BURST:
        result = a->iRemainBurst - b->iRemainBurst;
        break;
    case BY_BURST:
        result = a->iBurst - b->iBurst;
        break;
    }
    return (result == 0 ? compareProcess(a, b, c - 1) : result);
}

void swapProcess(PCB **a, PCB **b)
{
    PCB *temp = *a;
    *a = *b;
    *b = temp;
}

int checkPCBStart(PCB *p)
{
    if (!p)
        return -1;
    else
        return (p->iStart == -1 ? 0 : 1);
}

void setPCBStart(PCB *p, int t)
{
    p->iStart = t;
    p->iResponse = p->iStart - p->iArrival;
}

void setPCBFinish(PCB *p, int t)
{
    p->iFinish = t;
    p->iTaT = p->iFinish - p->iArrival;
    p->iWaiting = p->iTaT - p->iBurst;
}

int partition(PCB *arr[], int l, int h, int c)
{
    int i = l - 1;
    for (int j = l; j < h; j++)
        if (compareProcess(arr[j], arr[h], c) < 0)
            swapProcess(&arr[++i], &arr[j]);

    swapProcess(&arr[i + 1], &arr[h]);
    return i + 1;
}

void quickSort(PCB *arr[], int l, int h, int c)
{
    if (l < h)
    {
        int pi = partition(arr, l, h, c);
        quickSort(arr, l, pi - 1, c);
        quickSort(arr, pi + 1, h, c);
    }
}

void calculateAWT(int n, PCB *arr[])
{
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += arr[i]->iWaiting;

    printf("Average Waiting Time: %.2f\n", (float)sum / n);
}

void calculateATaT(int n, PCB *arr[])
{
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += arr[i]->iTaT;

    printf("Average Turnaround Time: %.2f\n", (float)sum / n);
}