#include "custom_def.h"

#include "dhry.h"

/* Global Variables: */

Rec_Pointer     Ptr_Glob,
                Next_Ptr_Glob;
int             Int_Glob;
Boolean         Bool_Glob;
char            Ch_1_Glob,
                Ch_2_Glob;
int             Arr_1_Glob [50];
int             Arr_2_Glob [50] [50];

#define REG	register
	
#ifndef REG
        Boolean Reg = false;
#define REG
        /* REG becomes defined as empty */
        /* i.e. no register variables   */
#else
        Boolean Reg = true;
#endif

/* variables for time measurement: */

#ifdef TIMES
struct tms      time_info;
extern  int     times (void);
                /* see library function "times" */
#define Too_Small_Time (2*HZ)
                /* Measurements should last at least about 2 seconds */
#endif
#ifdef TIME
extern long     time(long *);
                /* see library function "time"  */
#define Too_Small_Time 2
                /* Measurements should last at least 2 seconds */
#endif
#ifdef MSC_CLOCK
//extern clock_t clock(void);
#define Too_Small_Time (2*HZ)
#endif

long            Begin_Time,
                End_Time,
                User_Time;
float           Microseconds,
                Dhrystones_Per_Second;

/* end of variables for time measurement */

//#pragma clang section text="RAM_CODE"
RAM_FUNC void Proc_1 (Rec_Pointer Ptr_Val_Par)
/******************/
    /* executed once */
{
  REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;
                                        /* == Ptr_Glob_Next */
  /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
  /* corresponds to "rename" in Ada, "with" in Pascal           */

  structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob);
  Ptr_Val_Par->variant.var_1.Int_Comp = 5;
  Next_Record->variant.var_1.Int_Comp = Ptr_Val_Par->variant.var_1.Int_Comp;
  Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
  Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp == Ptr_Glob->Ptr_Comp */
  if (Next_Record->Discr == Ident_1)
    /* then, executed */
  {
    Next_Record->variant.var_1.Int_Comp = 6;
    Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp,
           &Next_Record->variant.var_1.Enum_Comp);
    Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
    Proc_7 (Next_Record->variant.var_1.Int_Comp, 10,
           &Next_Record->variant.var_1.Int_Comp);
  }
  else /* not executed */
    structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
} /* Proc_1 */

RAM_FUNC void Proc_2 (One_Fifty *Int_Par_Ref)
/******************/
    /* executed once */
    /* *Int_Par_Ref == 1, becomes 4 */
{
  One_Fifty  Int_Loc;
  Enumeration   Enum_Loc;

  Int_Loc = *Int_Par_Ref + 10;
  do /* executed once */
    if (Ch_1_Glob == 'A')
      /* then, executed */
    {
      Int_Loc -= 1;
      *Int_Par_Ref = Int_Loc - Int_Glob;
      Enum_Loc = Ident_1;
    } /* if */
    while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */

RAM_FUNC void Proc_3 (Rec_Pointer *Ptr_Ref_Par)
/******************/
    /* executed once */
    /* Ptr_Ref_Par becomes Ptr_Glob */
{
  if (Ptr_Glob != Null)
    /* then, executed */
    *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
  Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */

RAM_FUNC void Proc_4 (void) /* without parameters */
/*******/
    /* executed once */
{
  Boolean Bool_Loc;

  Bool_Loc = Ch_1_Glob == 'A';
  Bool_Glob = Bool_Loc | Bool_Glob;
  Ch_2_Glob = 'B';
} /* Proc_4 */

RAM_FUNC void Proc_5 (void) /* without parameters */
/*******/
    /* executed once */
{
  Ch_1_Glob = 'A';
  Bool_Glob = false;
} /* Proc_5 */

RAM_FUNC void dhry_main(uint32_t freq) {
	One_Fifty       Int_1_Loc;
  REG One_Fifty   Int_2_Loc;
  One_Fifty       Int_3_Loc;
  REG char        Ch_Index;
  Enumeration     Enum_Loc;
  Str_30          Str_1_Loc;
  Str_30          Str_2_Loc;
  REG int         Run_Index;
  REG int         Number_Of_Runs;
	
  /* Initializations */
  Next_Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));

  Ptr_Glob->Ptr_Comp                    = Next_Ptr_Glob;
  Ptr_Glob->Discr                       = Ident_1;
  Ptr_Glob->variant.var_1.Enum_Comp     = Ident_3;
  Ptr_Glob->variant.var_1.Int_Comp      = 40;
  strcpy (Ptr_Glob->variant.var_1.Str_Comp,
          "DHRYSTONE PROGRAM, SOME STRING");
  strcpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

  Arr_2_Glob [8][7] = 10;
        /* Was missing in published program. Without this statement,    */
        /* Arr_2_Glob [8][7] would have an undefined value.             */
        /* Warning: With 16-Bit processors and Number_Of_Runs > 32000,  */
        /* overflow may occur for this array element.                   */

  PRINTF("\n");
  PRINTF("Dhrystone Benchmark, Version 2.1 (Language: C)\n");
  PRINTF("\n");
  if (Reg) {
    PRINTF("Program compiled with 'register' attribute\n");
    PRINTF("\n");
  } else {
    PRINTF("Program compiled without 'register' attribute\n");
    PRINTF("\n");
  }
	
  PRINTF("Please give the number of runs through the benchmark: ");
  {
//    int n = 100000;
//    scanf ("%d", &n);
    Number_Of_Runs = RUN_NUMBER;
  }
  PRINTF("\n");

  PRINTF("Execution starts, %d runs through Dhrystone\n", Number_Of_Runs);
  /***************/
  /* Start timer */
  /***************/

#ifdef TIMES
  times (&time_info);
  Begin_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  Begin_Time = time ( (long *) 0);
#endif
#ifdef MSC_CLOCK
  Begin_Time = HAL_GetTick();
#endif

  for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index) {
    Proc_5();
    Proc_4();
      /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
    Int_1_Loc = 2;
    Int_2_Loc = 3;
    strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
    Enum_Loc = Ident_2;
    Bool_Glob = ! Func_2 (Str_1_Loc, Str_2_Loc);
      /* Bool_Glob == 1 */
    while (Int_1_Loc < Int_2_Loc)  /* loop body executed once */
    {
      Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
        /* Int_3_Loc == 7 */
      Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
        /* Int_3_Loc == 7 */
      Int_1_Loc += 1;
    } /* while */
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
      /* Int_Glob == 5 */
    Proc_1 (Ptr_Glob);
    for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
                             /* loop body executed twice */
    {
      if (Enum_Loc == Func_1 (Ch_Index, 'C'))
         /* then, not executed */
      {
        Proc_6 (Ident_1, &Enum_Loc);
        strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
        Int_2_Loc = Run_Index;
        Int_Glob = Run_Index;
      }
    }
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Int_2_Loc = Int_2_Loc * Int_1_Loc;
    Int_1_Loc = Int_2_Loc / Int_3_Loc;
    Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
      /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
    Proc_2 (&Int_1_Loc);
      /* Int_1_Loc == 5 */

  } /* loop "for Run_Index" */

  /**************/
  /* Stop timer */
  /**************/

#ifdef TIMES
  times (&time_info);
  End_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  End_Time = time ( (long *) 0);
#endif
#ifdef MSC_CLOCK
  End_Time = HAL_GetTick();
#endif

  PRINTF("Execution ends\n");
  PRINTF("\n");
  PRINTF("Final values of the variables used in the benchmark:\n");
  PRINTF("\n");
  PRINTF( "Int_Glob:            %d\n", Int_Glob);
	PRINTF("        should be:   %d\n", 5);
	PRINTF( "Bool_Glob:           %d\n", Bool_Glob);
	PRINTF( "        should be:   %d\n", 1);
	PRINTF("Ch_1_Glob:           %c\n", Ch_1_Glob);
	PRINTF("        should be:   %c\n", 'A');	
  PRINTF("Ch_2_Glob:           %c\n", Ch_2_Glob);	
  PRINTF("        should be:   %c\n", 'B');	
  PRINTF("Arr_1_Glob[8]:       %d\n", Arr_1_Glob[8]);	
  PRINTF("        should be:   %d\n", 7);	
  PRINTF("Arr_2_Glob[8][7]:    %d\n", Arr_2_Glob[8][7]);	
  PRINTF("        should be:   Number_Of_Runs + 10\n");
  PRINTF("Ptr_Glob->\n");
  PRINTF("  Ptr_Comp:          %d\n", (int) Ptr_Glob->Ptr_Comp);	
  PRINTF("        should be:   (implementation-dependent)\n");
  PRINTF("  Discr:             %d\n", Ptr_Glob->Discr);	
	PRINTF("        should be:   %d\n", 0);	
  PRINTF("  Enum_Comp:         %d\n", Ptr_Glob->variant.var_1.Enum_Comp);	
	PRINTF("        should be:   %d\n", 2);	
  PRINTF("  Int_Comp:          %d\n", Ptr_Glob->variant.var_1.Int_Comp);	
  PRINTF("        should be:   %d\n", 17);	
  PRINTF("  Str_Comp:          %s\n", Ptr_Glob->variant.var_1.Str_Comp);	
  PRINTF("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  PRINTF("Next_Ptr_Glob->\n");
  PRINTF("  Ptr_Comp:          %d\n", (int) Next_Ptr_Glob->Ptr_Comp);	
  PRINTF("        should be:   (implementation-dependent), same as above\n");
	PRINTF("  Discr:             %d\n", Next_Ptr_Glob->Discr);	
  PRINTF("        should be:   %d\n", 0);	
  PRINTF("  Enum_Comp:         %d\n", Next_Ptr_Glob->variant.var_1.Enum_Comp);	
  PRINTF("        should be:   %d\n", 1);	
  PRINTF("  Int_Comp:          %d\n", Next_Ptr_Glob->variant.var_1.Int_Comp);	
  PRINTF("        should be:   %d\n", 18);
  PRINTF("  Str_Comp:          %s\n",
                                Next_Ptr_Glob->variant.var_1.Str_Comp);	
  PRINTF("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  PRINTF("Int_1_Loc:           %d\n", Int_1_Loc);	
  PRINTF("        should be:   %d\n", 5);	
  PRINTF("Int_2_Loc:           %d\n", Int_2_Loc);	
  PRINTF("        should be:   %d\n", 13);	
  PRINTF("Int_3_Loc:           %d\n", Int_3_Loc);	
  PRINTF("        should be:   %d\n", 7);	
  PRINTF("Enum_Loc:            %d\n", Enum_Loc);	
  PRINTF("        should be:   %d\n", 1);	
  PRINTF("Str_1_Loc:           %s\n", Str_1_Loc);	
  PRINTF("        should be:   DHRYSTONE PROGRAM, 1'ST STRING\n");
  PRINTF("Str_2_Loc:           %s\n", Str_2_Loc);	
  PRINTF("        should be:   DHRYSTONE PROGRAM, 2'ND STRING\n");
  PRINTF("\n");

  User_Time = End_Time - Begin_Time;

  if (User_Time < Too_Small_Time) {
		PRINTF( "Measured time too small to obtain meaningful results %ld-%ld\n",
		Begin_Time, End_Time);
    PRINTF("Please increase number of runs\n");
  } else {
#ifdef TIME
    Microseconds = (float) User_Time * Mic_secs_Per_Second
                        / (float) Number_Of_Runs;
    Dhrystones_Per_Second = (float) Number_Of_Runs / (float) User_Time;
#else
    Microseconds = (float) User_Time * (float)Mic_secs_Per_Second
                        / ((float) HZ * ((float) Number_Of_Runs));
    Dhrystones_Per_Second = ((float) HZ * (float) Number_Of_Runs)
                        / (float) User_Time;
#endif
		PRINTF("MicroSecond for one run through Dhrystone[%ld-%ld]:\t %4.3f \n",
		Begin_Time, End_Time, Microseconds);
	
    PRINTF("Dhrystones per Second:\t%4.3f \n", Dhrystones_Per_Second);
		//DMIPS/MHz = 10^6 / (1757 * Number of processor clock cycles per Dhrystone loop)
		PRINTF("DMIPS/MHz:\t%4.3f\n", Dhrystones_Per_Second /(1757 * (freq/1000000)));
  }
}
#pragma clang section text="" // Reset back to default flash execution
