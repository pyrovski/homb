/*
This is the source for QUIVER-Lib, Qualitative Inference from Variance in Execution Rate Library.  It is a wrapper around standard MPI calls that records timing information.  

Inspired by Joel Welling (welling@psc.edu)
Written by Max Hutchinson (mhutchin@psc.edu)
*/

/* Include Files */ 
#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Book-keeping */
#define DOWN 100
#define UP 101
#define ROOT 0
#define MAX(x,y) (((x) > (y))? x : y)
#define MIN(x,y) (((x) > (y))? y : x)

int QMPI_Init(int *argc, char *argv[]){
  /* Create matrix for times */
  double **times = malloc(nPEs * sizeof(double *)); 
  for (iPE = 0; iPE < nPEs; iPE++)
    times[iPE] = malloc(NITER * sizeof(double));

  return 0;
}

int QMPI_Finalize(){

  return 0;
}

int QMPI_Reduce









    /* Begin Timing */
    startTime = MPI_Wtime();

    /* Do extra floating point ops (if wanted) */    
    if (EXTRA != 0 ){
      extraCalculations(t);
    }

    /* Wait if we told you to... */
    if (myPE == waiter && iter <= duration){
      sleep(delay);
    }

    /* Do all the real work */
    dtg = work(t, &new, myPE, request, status);

    /* End timing */
    endTime = MPI_Wtime();

    /* Store Time */
    times[myPE][iter-1] = endTime-startTime;
  }

  /* Sum the diagonal values across PEs (error checker) */
  sumTrace(t, new, myPE, &sum);
  MPI_Reduce(&sum, &sumg, 1, MPI_FLOAT, MPI_SUM, ROOT, MPI_COMM_WORLD);

  /* Sync everything up before the anaysis stage of the program */
  MPI_Barrier(MPI_COMM_WORLD);

  /* Gather iteration runtimes to ROOT's matrix */
  timeUpdate(times, myPE);

  /* Run statistics on times (Root only) */
  if (myPE == ROOT) {
    statistics(times, covar, &minTime, &meanTime, &maxTime, &stdvTime, &NstdvTime);
  }

  /* Output */
  if (myPE == ROOT) {
    /* Check for file output */
    if ( (fOut + tOut) > 0 )
      fileIO(times, covar, minTime, meanTime, maxTime, NstdvTime);
    /* Check for Standard Output */
    if ( (sOut + vOut) > 0 )
      stdoutIO(times, covar, minTime, meanTime, maxTime, NstdvTime);
  }

  /* Finalize */
  MPI_Finalize();
}


/*********** FUNCTIONS ***********/

void initContext(int argc, char *argv[]){
  /* Allocate space for file names */
  fileOutName = malloc(100 * sizeof(char));
  timesOutName = malloc(100 * sizeof(char));

  /* "Logicals" for file output, summary standard output, and full standard output */
  vOut = 0; sOut = 0; fOut = 0; tOut = 0; pHeader = 1; pContext = 0;

  /* Location, size, and duration of delayed task */
  waiter = -1; delay = 0.; duration = 0;

  /* Default required values */
  NR = 8192; NC = 8192; NITER = 20; EXTRA = 0;  nThreads = 1; nPEs = 1;

  /* Default with order */
  disorder = 0; shuffle = 0;

  /* Cycle through command line args */
  for (int i = 1; i < argc; i++){  
    /* Look for number of rows */
    if ( strcmp("-NR", argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&NR);
    }
    
    /* Look for number of columns */  
    if ( strcmp("-NC", argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&NC);
    }

    /* Look for number of rows and columns */  
    if ( strcmp("-NRC", argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&NR);
      sscanf(argv[i+1],"%d",&NC);
    }
    
    /* Look for number of iterations */
    if ( strcmp("-NITER", argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&NITER);
    }

    /* Look for number of extra flop */
    if ( strcmp("-EXTRA", argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&EXTRA);
    }

    /* Look for task delay */
    if ( strcmp("-delay",argv[i]) == 0 ){
      sscanf(argv[i+1],"%d",&waiter);
      sscanf(argv[i+2],"%f",&delay);
      sscanf(argv[i+3],"%d",&duration);
    }

    /* Look for "Disorder" option */ 
    if ( strcmp("-disorder",argv[i]) == 0 ){
      disorder = 1;
    }

    /* Look for "Shuffle" option */ 
    if ( strcmp("-shuffle",argv[i]) == 0 ){
      shuffle = 1;
    }

    /* Look for "verbose" output filename */
    if ( strcmp("-vf",argv[i]) == 0 ){
      sscanf(argv[i+1],"%s",fileOutName);
      fOut = 1;
    }

    /* Look for time table file output */
    if ( strcmp("-tf",argv[i]) == 0 ){
      sscanf(argv[i+1],"%s",timesOutName);
      tOut = 1;
    }
    
    /* Look for "summary" standard out */
    if ( strcmp("-s",argv[i]) == 0 ){
      sOut = 1;
    }

    /* Look for "verbose" standard out */
    if ( strcmp("-v",argv[i]) == 0 ){
      vOut = 1;
    }
    
    /* Look for "No Header" option */
    if ( strcmp("-nh",argv[i]) == 0 ){
      pHeader = 0;
    }

    /* Look for "verbose" standard out */
    if ( strcmp("--help",argv[i]) == 0 ){
      printf("Usage: ./homb {-NR num-rows -NC num-cols} | {-NRC num-rows num-calls} -NITER num-iter [-EXTRA extra-flop] [-delay rank time(s) duration(iterations)] \n \
             [-v | -s] [-vf verbose-out-filename] [-cf covar-out-filename] [-tf time-out-filename] [-help] \n");
      exit(EXIT_SUCCESS);
    }

    /* Look for "Print Context" option */
    if ( strcmp("-pc",argv[i]) == 0 ){
      pContext = 1;
    }
  }

  /* Check if NITER is large enough */
  if (NITER < 3){
    fprintf(stderr,"NITER is too small, must be > 3 \n");
    exit(EXIT_FAILURE);
  }
}

void setPEsParams(int *myPE) {
  /* Find nPEs */
  MPI_Comm_size(MPI_COMM_WORLD, &nPEs);

  /* Find myPE */
  MPI_Comm_rank(MPI_COMM_WORLD, myPE); 

  /* Re-organized PEs (if you want) */
  if (shuffle)
    *myPE= ( *myPE / 2) + ( *myPE % 2) * ( nPEs / 2);

  /* Find number of threads per task */
  #pragma omp parallel shared(nThreads)
  {
    #pragma omp single
      nThreads = omp_get_num_threads();
  }

  /* Check to make sure matrix breaks into nPEs evenly sized peices */
  if ( NR %(nPEs)!=0) {
    MPI_Finalize();
    if (*myPE == 0) {
      fprintf(stderr, "The example is only for factors of %d \n", NR);
    }
    exit(EXIT_FAILURE);
  }

  /* Set distributed grid size */
  nrl = NR/nPEs;
}

float*** createMatrix() {
  /* Returned variable */
  float ***aux;

  /* Iterators */
  int plane, row;

  /* Allocate 2 pointers, one for new and one for old values */
  aux = malloc(2 * sizeof(float *));

  /* Allocate nrl+2 pointers, one for each row of each grid */
  aux[0] = malloc((nrl+2) * sizeof(float *));
  aux[1] = malloc((nrl+2) * sizeof(float *));

  /* Check */
  if (aux == NULL) {
    printf("\nFailure to allocate memory for row pointers.\n");
    exit(EXIT_FAILURE);
  }

  /* Allocate the rows */
  for (plane = 0; plane < 2; plane++)
    for (row = 0; row < (nrl+2); row++) {
      aux[plane][row] = malloc((NC+2) * sizeof(float));
  
      /* Check */
      if (aux[plane][row] == NULL) {
        printf("\nFailure to allocate memory for row %d\n",row);
        exit(EXIT_FAILURE);
      }
    }

  return aux;
}

void initializeMatrix(float ***t, int myPE, int *new) {
  /* Local boundry conditions */
  float tMin, tMax;

  /* Iterators */
  int i, j;

  /* pick which of two grids is "new" */
  *new = 1;
   
  /* Initialize everything to zero */
  for (i = 0; i <= nrl+1; i++)
    for (j = 0; j <= NC+1; j++)
      t[1][i][j] = 0.0;

  /* Lower BC bound */
  tMin = (myPE)*100.0/nPEs;

  /* Upper BC bound */
  tMax = (myPE+1)*100.0/nPEs;

  /* Left and Right boundaries */
  for (i = 0; i <= nrl+1; i++) {
    t[1][i][0] = 0.0;
    t[1][i][NC+1] = tMin + ((tMax-tMin)/nrl)*i;
  }

  /* Top boundary */
  if (myPE == 0)
    for (j = 0; j <= NC+1; j++)
      t[1][0][j] = 0.0;

  /* Bottom boundary */
  if (myPE == nPEs-1)
    for (j = 0; j <= NC+1; j++)
      t[1][nrl+1][j] = (100.0/NC) * j;

  /* Copy to other grid */
  for (i = 0; i <= nrl+1; i++)
    for (j = 0; j <= NC+1; j++)
      t[0][i][j]=t[1][i][j];
}

void printContext(void){
  printf("Number of Rows: %d, Number of Columns: %d, Number of Iterations: %d \n", NR, NC, NITER);
  printf("Number of Tasks: %d, Number of Threads per Task: %d \n", nPEs, nThreads);
  if (EXTRA != 0)
    printf("Number of Extra Floating Point Operations per Iteration: %d \n", EXTRA);
  if (delay != 0)   
    printf("Task Number %d Delayed %f s per Iteration \n", waiter, delay);
  if (disorder != 0)
    printf("Array indexes were disordered to fool the prefetcher \n");
  if (shuffle != 0)
    printf("PE Numbers were shuffled so messages travel further\n");
  if (vOut)
    printf("Verbose Standard Output \n");
  else if (sOut)
    if (pHeader)
      printf("Summary Standard Ouput with Header \n");
    else
      printf("Summary Standard Output without Header \n");
  if (fOut)
    printf("Verbose File Output to: %s \n", fileOutName);
  if (tOut)
    printf("Times Matrix File Output to: %s \n", timesOutName);
}

void extraCalculations(float ***t){
  /* Iterators */
  int x, y, z;

  /* Loop over grid doing nothing (for extra FLOP) */
  #pragma omp parallel 
  {
  for (x = 1; x <= nrl; x++)
    for (y = 1; y <= NC; y++)
      for (z = 0; z <= EXTRA; z++) {
        t[1][x][y] += z;  // Do
        t[1][x][y] -= z;  // Undo
      }
  }
}

float work(float ***t, int *new, int myPE, MPI_Request request[], MPI_Status status[]){
  /* Application measurments */
  float d = 0., dt = 0., dtg = 0., stat = 0.;

  /* Indicies */
  int old = 1 - *new;

  /* Iterators */
  int i, j, jj, k, kp, kpp;
  		
  /* Begin parallel region if there are more than 1 thread per task */
  #pragma omp parallel if (nThreads != 1) shared(t, new, old, nrl, dt, NR, NC, NITER) private(d)
  {
    /* Use master thread to calculate and communicate boundies */
    #pragma omp master
    {
      /* Loop over top and bottom boundry */
      for (k = 1; k <= NC; k++){
        /*Calculate average of neighbors as new value (Point Jacobi method) */
        t[*new][1][k] = 0.25 * (t[old][2][k] + t[old][0][k] + t[old][1][k+1] + t[old][1][k-1]);
        t[*new][nrl][k] = 0.25 * (t[old][nrl+1][k] + t[old][nrl-1][k] + t[old][nrl][k+1] + t[old][nrl][k-1]);
        /* Calculate local maximum change from last step */
        d = MAX(fabs(t[*new][1][k] - t[old][1][k]), d);  /* Puts thread's max in d */
        d = MAX(fabs(t[*new][nrl][k] - t[old][nrl][k]), d);  /* Puts thread's max in d */
      }
      if (nPEs!=1){
        /* Exchange boundries with neighbor tasks */
        if (myPE < nPEs-1 )
          /* Sending Down; Only npes-1 do this */
          MPI_Isend(&(t[*new][nrl][1]), NC, MPI_FLOAT, myPE+1, DOWN, MPI_COMM_WORLD, &request[0]);
        if (myPE != 0)
         /* Sending Up; Only npes-1 do this */
         MPI_Isend(&t[*new][1][1], NC, MPI_FLOAT, myPE-1, UP, MPI_COMM_WORLD, &request[1]);
        if (myPE != 0)
          /* Receive from UP */
          MPI_Irecv(&t[*new][0][1], NC, MPI_FLOAT, MPI_ANY_SOURCE, DOWN, MPI_COMM_WORLD, &request[2]);
        if (myPE != nPEs-1)
          /* Receive from DOWN */
          MPI_Irecv(&t[*new][nrl+1][1], NC, MPI_FLOAT, MPI_ANY_SOURCE, UP, MPI_COMM_WORLD, &request[3]);
      }
    }

    /* Everyone calculates values and finds local max change */
    #pragma omp for schedule(runtime) nowait
      for (i = 2; i <= nrl-1; i++)
        for (j = 0; j < NC / 16; j++){
          jj = j / 2 + (j % 2) * NC / 32 ;
	  for (k = 1; k <= 16; k++){ 
            kp = k + jj * 16;
            kpp = k + j * 16;
            if (disorder){
              t[*new][i][kp] = 0.25 * (t[old][i+1][kp] + t[old][i-1][kp] + t[old][i][kp+1] + t[old][i][kp-1]);
              d = MAX(fabs(t[*new][i][kp] - t[old][i][kp]), d);
            }else{
              t[*new][i][kpp] = 0.25 * (t[old][i+1][kpp] + t[old][i-1][kpp] + t[old][i][kpp+1] + t[old][i][kpp-1]);
              d = MAX(fabs(t[*new][i][kpp] - t[old][i][kpp]), d);
            }
          }
        }

    /*Local max change become taks-global max change */    
    #pragma omp critical
      dt = MAX(d, dt); /* Finds max of the d's */
  }

  /* If there are multiple tasks, wait for the rest and find global max change */
  if (nPEs!=1){
    if (myPE != nPEs-1 )
      MPI_Wait(&request[0], &status[0]); 
    if (myPE != nPEs-1)
      MPI_Wait(&request[3], &status[3]);
    if (myPE != 0)
      MPI_Wait(&request[1], &status[1]);
    if (myPE != 0)
      MPI_Wait(&request[2], &status[2]);

    /* Find max dt over the whole domain */
    MPI_Reduce(&dt, &dtg, 1, MPI_FLOAT, MPI_MAX, ROOT, MPI_COMM_WORLD);
    stat=dtg;
  }
  else
    stat=dt;
  
  /* "Flip" pointers */
  *new = old;
  stat=stat+1.0-1.0; //Keep this line! (prevents harmful optimization)

  return stat;
}

void sumTrace(float ***t, int new, int myPE, float *sum) {
  int jOff, i;
  jOff = myPE*nrl;
  *sum = 0.;
  int old = 1 - new;
  
  /* Sum over diagonal with knowledge of distributed grids */
  for (i = 1; i <= nrl; i++){
    *sum += t[old][i][jOff+i];
  }
  return;
}

void timeUpdate(double **times, int myPE){
  /* Update Root's times matrix to include all times */
  if (myPE != ROOT){
    MPI_Request req;
    MPI_Status sta;

    /* Sending to ROOT; Only npes-1 do this */
    MPI_Isend(&times[myPE][0], NITER, MPI_DOUBLE, ROOT, myPE, MPI_COMM_WORLD, &req);
    /* Waiting for Root */
    MPI_Wait(&req, &sta);
  }

  if (myPE == ROOT){
    MPI_Request rootRequest[nPEs];
    MPI_Status rootStatus[nPEs];    

    /* Recieving times from other tasks */ 
    for (int iPE = 1; iPE < nPEs; iPE++){
      MPI_Irecv(&times[iPE][0], NITER, MPI_DOUBLE, MPI_ANY_SOURCE, iPE, MPI_COMM_WORLD, &rootRequest[iPE]);
      MPI_Wait(&rootRequest[iPE], &rootStatus[iPE]);
    }
  }
}

void statistics(double **times, double **covar, double *minTime, double *meanTime, double *maxTime, double *stdvTime, double *NstdvTime){
  double temp;
  /* Compute mean, max, min of times */
  for (int iPE = 0; iPE < nPEs; iPE++)
    for (int iter=2; iter<NITER; iter++){
      *meanTime += times[iPE][iter];
      *maxTime = MAX(*maxTime, times[iPE][iter]);
      *minTime = MIN(*minTime, times[iPE][iter]);
    }
  *meanTime = *meanTime / (NITER - 2) / nPEs;

  /* Compute standard deviation of times */
  for (int iPE = 0; iPE < nPEs; iPE++)
    for (int iter = 2; iter < NITER; iter++){
      *stdvTime += ( times[iPE][iter] - *meanTime ) * ( times[iPE][iter] - *meanTime );
    }
  *stdvTime = sqrt(*stdvTime / (NITER - 2) / nPEs);

  /* Normalized standard deviation (stdv / mean) */
  *NstdvTime = *stdvTime / *meanTime;
}

void fileIO(double **times, double **covar, double minTime, double meanTime, double maxTime, double NstdvTime){
  FILE *outFile, *timesOutFile;
  int iter, iPE, jPE;

  if (fOut){
    outFile = fopen(fileOutName,"w");
    /* Print heading and summary data */
    fprintf(outFile,"#==========================================================================================================#\n");
    fprintf(outFile,"#\tTasks\tThreads\tNR\tNC\tNITER\tmeanTime \tmaxTime  \tminTime  \tNstdvTime  #\n");
    fprintf(outFile,"#==========================================================================================================#\n");
    fprintf(outFile,"\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\n",nPEs,nThreads,NR,NC,NITER, meanTime, maxTime, minTime, NstdvTime);

    fprintf(outFile,"\n");
    fprintf(outFile,"\n");

    /* Print full times matrix */
    fprintf(outFile,"# Full Time Output (rows are times, cols are tasks)\n");
    for (iter = 0; iter < NITER; iter++) {
      for (iPE = 0; iPE < nPEs; iPE++)
        fprintf(outFile,"%e \t",times[iPE][iter]);
        fprintf(outFile,"\n");
      }

    /* Close file */
    fclose(outFile);
  }

  if (tOut){
    timesOutFile = fopen(timesOutName,"w");
    /* Print full times matrix */
    fprintf(timesOutFile,"# Full Time Output (rows are times, cols are tasks)\n");
    for (iter = 0; iter < NITER; iter++) {
      for (iPE = 0; iPE < nPEs; iPE++)
        fprintf(timesOutFile,"%e \t",times[iPE][iter]);
        fprintf(timesOutFile,"\n");
      }
    /* Close file */
    fclose(timesOutFile);
  }
}

void stdoutIO(double **times, double **covar, double minTime, double meanTime, double maxTime, double NstdvTime){
  int iter, iPE, jPE;

  /* Same data as above, just to Standard Output */
  if (pHeader){
    printf("#==========================================================================================================#\n");
    printf("#\tTasks\tThreads\tNR\tNC\tNITER\tmeanTime \tmaxTime  \tminTime  \tNstdvTime  #\n");
    printf("#==========================================================================================================#\n");
  }
  printf("\t%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\n",nPEs,nThreads,NR,NC,NITER, meanTime, maxTime, minTime, NstdvTime);

  /* Only if "Verbose Output" asked for */
  if (vOut == 1){ 
    printf("\n");
    printf("\n");

    printf("# Full Time Output (rows are times, cols are tasks)\n");
    for (iter = 0; iter < NITER; iter++){ 
      for (iPE = 0; iPE < nPEs; iPE++)
        printf("%e \t",times[iPE][iter]);
      printf("\n");
      }
  }
}


/* Utilities to use if you want them */

/* Inverse of the erf function.  Use it to get normaly destributed random numbers */
float ierf(int res){
  float LB, UB, MP, MPval, randomNum;
  int i;

  srand48( (unsigned int) time(NULL) );
  randomNum = (float) (drand48() * 2. - 1.);

  if(randomNum < 0.0)
    LB = -1.0;  UB = 0.0; MP = -.5;

  if (randomNum > 0.0)
    LB = 0.0; UB = 1.0; MP = .5;

  for (i=0; i<res; i++){
    MPval = erf(MP);

    if (randomNum < MPval){
      UB = MP;
      MP = (LB + UB) / 2.0;
    }
    else{
      LB = MP;
      MP = (LB + UB) / 2.0;
    }
  }
  return MP;
}

/* Will print whole matrix if you'd like.  ***t is the pointer the the matrix, plane is either 0 or 1, and nrl is the local grid size */
void printMatrix(float ***t, int plane){
  int i, j;
  printf("T Matrix: \n");
  for (i=0; i<nrl+2; i++)
  {    
    for (j=0; j<nrl+2; j++)
      printf("%10.5f  ", t[plane][i][j]); 
    printf("\n");
  }
  printf("\n");
}
