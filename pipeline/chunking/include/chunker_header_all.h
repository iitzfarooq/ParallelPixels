#pragma once

#include<pthread.h> 
#include<stdio.h>   
#include<unistd.h>  
#include<dirent.h>  
#include<stdbool.h> 
#include<string.h>  
#include<stdlib.h>  
#include<signal.h>

extern volatile sig_atomic_t stop_flag;