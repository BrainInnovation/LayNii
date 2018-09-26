
#include <stdio.h>
#include "nifti2_io.h"
//#include "nifti2.h"
//#include "nifti1.h"
//#include "nifticdf.h"
//#include "nifti_tool.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <string>
//#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics_double.h>
using namespace std;

#define PI 3.14159265; 


//#include "utils.hpp"

int show_help( void )
{
   printf(
      "LN_COLUMNAR_DIST : This program calculates cortical distances (a.k.a. columnar structures) based on the GM geometryn"
      "\n"
      "   the layer nii file and the landmarks nii file should have the same dimensions  \n"
      "\n"
      "    basic usage: LN_3DCOLUMNS -layer_file equi_dist_layers.nii -landmarks landmarks.nii \n"
      "\n"
      "\n"
      "   This program now supports INT16, INT32 and FLOAT23 \n"

      "\n"
      "       -help               	: show this help\n"
      "       -layer_file     		: nii file that contains layer or column masks \n"
      "       -landmarks            : nii file with landmarks (use value one as origin )  \n"
      "	                              Landmarks should be at least 4 voxels thick\n"
      "       -twodim 		     	: optional argument to run in 2 Dim only \n"
      "       -vinc                 : maximal length of cortical distance\n"
      "                              bigger values will take longer but span a \n"
      "	                             larger cortical area (default is 40)  \n"


      "\n");
   return 0;
}

int main(int argc, char * argv[])
{

   char       * layer_filename=NULL, * fout=NULL, * landmarks_filename=NULL ;
   int          ac, twodim=0, do_masking=0, vinc_max = 40 ; 
   if( argc < 3 ) return show_help();   // typing '-help' is sooo much work 

   // process user options: 4 are valid presently 
   for( ac = 1; ac < argc; ac++ ) {
      if( ! strncmp(argv[ac], "-h", 2) ) {
         return show_help();
      }
      else if( ! strcmp(argv[ac], "-layer_file") ) {
         if( ++ac >= argc ) {
            fprintf(stderr, "** missing argument for -layer_file\n");
            return 1;
         }
         layer_filename = argv[ac];  // no string copy, just pointer assignment 
      }
      else if( ! strcmp(argv[ac], "-landmarks") ) {
         if( ++ac >= argc ) {
            fprintf(stderr, "** missing argument for -input\n");
            return 1;
         }
         landmarks_filename = argv[ac];  // no string copy, just pointer assignment 
      }
      else if( ! strcmp(argv[ac], "-twodim") ) {
         twodim = 1;
         cout << " I will do smoothing only in 2D"  << endl; 
      }
     else if( ! strcmp(argv[ac], "-vinc") ) {
        if( ++ac >= argc ) {
            fprintf(stderr, "** missing argument for -vinc\n");
            return 1;
         }
         vinc_max = atof(argv[ac]);  // no string copy, just pointer assignment 
      }
      else {
         fprintf(stderr,"** invalid option, '%s'\n", argv[ac]);
         return 1;
      }
   }

   if( !landmarks_filename  ) { fprintf(stderr, "** missing option '-landmarks'\n");  return 1; }
   // read input dataset, including data 
   nifti_image * nim_landmarks_r = nifti_image_read(landmarks_filename, 1);
   if( !nim_landmarks_r ) {
      fprintf(stderr,"** failed to read layer NIfTI image from '%s'\n", landmarks_filename);
      return 2;
   }
   
   if( !layer_filename  ) { fprintf(stderr, "** missing option '-layer_file'\n");  return 1; }
   // read input dataset, including data 
   nifti_image * nim_layers_r = nifti_image_read(layer_filename, 1);
   if( !nim_layers_r ) {
      fprintf(stderr,"** failed to read layer NIfTI image from '%s'\n", layer_filename);
      return 2;
   }
   
      // get dimsions of input 
   int sizeSlice = nim_layers_r->nz ; 
   int sizePhase = nim_layers_r->nx ; 
   int sizeRead = nim_layers_r->ny ; 
   int nrep =  nim_layers_r->nt; 
   int nx =  nim_layers_r->nx;
   int nxy = nim_layers_r->nx * nim_layers_r->ny;
   int nxyz = nim_layers_r->nx * nim_layers_r->ny * nim_layers_r->nz;
   float dX =  nim_layers_r->pixdim[1] ; 
   float dY =  nim_layers_r->pixdim[2] ; 
   float dZ =  nim_layers_r->pixdim[3] ; 
   
   
   if  (twodim == 1) dZ = 1000 * dZ ; 
   
      
   //nim_mask->datatype = NIFTI_TYPE_FLOAT32;
   //nim_mask->nbyper = sizeof(float);
   //nim_mask->data = calloc(nim_mask->nvox, nim_mask->nbyper);
   
   
   nifti_image * nim_layers  	= nifti_copy_nim_info(nim_layers_r);
   nim_layers->datatype = NIFTI_TYPE_FLOAT32;
   nim_layers->nbyper = sizeof(float);
   nim_layers->data = calloc(nim_layers->nvox, nim_layers->nbyper);
   float  *nim_layers_data = (float *) nim_layers->data;
   
   nifti_image * nim_landmarks  	= nifti_copy_nim_info(nim_layers_r);
   nim_landmarks->datatype = NIFTI_TYPE_INT32;
   nim_landmarks->nbyper = sizeof(int);
   nim_landmarks->data = calloc(nim_landmarks->nvox, nim_landmarks->nbyper);
   int  *nim_landmarks_data = (int *) nim_landmarks->data;
   
   
   /////////////////////////////////////////////
   /////////  fixing potential problems with different input datatypes  //////////////
   /////////////////////////////////////////////

if ( nim_landmarks_r->datatype == NIFTI_TYPE_FLOAT32 ) {
  float  *nim_landmarks_r_data = (float *) nim_landmarks_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_landmarks_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (float) (*(nim_landmarks_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}  
  

if ( nim_landmarks_r->datatype == NIFTI_TYPE_INT16 ) {
  short  *nim_landmarks_r_data = (short *) nim_landmarks_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_landmarks_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (float) (*(nim_landmarks_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}    


if ( nim_landmarks_r->datatype == NIFTI_TYPE_INT32 ) {
  int  *nim_landmarks_r_data = (int *) nim_landmarks_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_landmarks_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (float) (*(nim_landmarks_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}    

if ( nim_layers_r->datatype == NIFTI_TYPE_FLOAT32 ) {
  float  *nim_layers_r_data = (float *) nim_layers_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_layers_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (int) (*(nim_layers_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}    
  
if ( nim_layers_r->datatype == NIFTI_TYPE_INT16 ) {
  short  *nim_layers_r_data = (short *) nim_layers_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_layers_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (int) (*(nim_layers_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}    

if ( nim_layers_r->datatype == NIFTI_TYPE_INT32 ) {
  int  *nim_layers_r_data = (int *) nim_layers_r->data;
  	for(int it=0; it<nrep; ++it){  
	  for(int islice=0; islice<sizeSlice; ++islice){  
	      for(int iy=0; iy<sizePhase; ++iy){
	        for(int ix=0; ix<sizeRead; ++ix){
        		 *(nim_layers_data  + nxyz *it +  nxy*islice + nx*ix  + iy  ) = (int) (*(nim_layers_r_data  + nxyz *it +  nxy*islice + nx*ix  + iy  )) ;	
           } 
	    }
	  }
	}
}    
	

   cout << " " << sizeSlice << " slices    " <<  sizePhase << " PhaseSteps     " <<  sizeRead << " Read steps    " <<  nrep << " timesteps "  << endl; 
   cout << " Voxel size    " <<  dX << " x " <<  dY << " x "  <<  dZ  << endl; 

	
	cout << " datatye of Layers mask = " << nim_layers->datatype << endl;
    cout << " datatye of landmark mask = " << nim_landmarks ->datatype << endl;

///////////////////////////////////
////Finding number of layers  /////
///////////////////////////////////
 int layernumber = 0 ;

	for(int iz=0; iz<sizeSlice; ++iz){
  		for(int iy=0; iy<sizePhase; ++iy){
       		for(int ix=0; ix<sizeRead; ++ix){
	 		 if (*(nim_layers_data   +  nxy*iz + nx*ix  + iy  ) > layernumber)  layernumber = *(nim_layers_data   +  nxy*iz + nx*ix  + iy) ; 
	  		  
      		} 
   		} 
   	}
   	
cout << " There are  " <<  layernumber<< " layers  " << endl; 

///////////////////////////////////
////    allocating necessary files  /////
///////////////////////////////////

    
    nifti_image * Grow_x  	= nifti_copy_nim_info(nim_layers);
    nifti_image * Grow_y  	= nifti_copy_nim_info(nim_layers);
    nifti_image * Grow_z  	= nifti_copy_nim_info(nim_layers);

    Grow_x->datatype 		= NIFTI_TYPE_INT16; 
	Grow_y->datatype 		= NIFTI_TYPE_INT16;
	Grow_x->datatype 		= NIFTI_TYPE_INT16;

    Grow_x->nbyper 		= sizeof(short);
	Grow_y->nbyper 		= sizeof(short);
	Grow_z->nbyper 		= sizeof(short);

    Grow_x->data = calloc(Grow_x->nvox, Grow_x->nbyper);
    Grow_y->data = calloc(Grow_y->nvox, Grow_y->nbyper);
    Grow_z->data = calloc(Grow_z->nvox, Grow_z->nbyper);

    short  *Grow_x_data = (short *) Grow_x->data;
    short  *Grow_y_data = (short *) Grow_y->data;
    short  *Grow_z_data = (short *) Grow_z->data;

 //   cout << " not Segmentation fault until here 272" << endl; 

	nifti_image * growfromCenter 		= nifti_copy_nim_info(nim_layers);
	nifti_image * growfromCenter_thick 	= nifti_copy_nim_info(nim_layers);

    growfromCenter->datatype 			= NIFTI_TYPE_INT16; 
    growfromCenter_thick->datatype 		= NIFTI_TYPE_INT16; 

    growfromCenter->nbyper 				= sizeof(short);
    growfromCenter_thick->nbyper 		= sizeof(short);

    growfromCenter->data 		= calloc(growfromCenter->nvox, growfromCenter->nbyper);
    growfromCenter_thick->data 	= calloc(growfromCenter_thick->nvox, growfromCenter_thick->nbyper);

    short  *growfromCenter_data 		= (short *) growfromCenter->data;
    short  *growfromCenter_thick_data 	= (short *) growfromCenter_thick->data;

    
 //   cout << " not Segmentation fault until here 292" << endl; 



//cout << " not Segmentation fault until here 306" << endl; 


///////////////////////////////////
////   prepare growing variables  /////
///////////////////////////////////

float x1g = 0.;
float y1g = 0.;
float z1g = 0.;

float dist_min2 = 0.;
float dist_i = 0.; 
float dist_p1 = 0.;

int grow_vinc = 3 ;
int grow_vinc_area = 1 ;
//int vinc_max = 40 ; 
float dist (float x1, float y1, float z1, float x2, float y2, float z2, float dX, float dY, float dZ ) ;



///////////////////////////////////
////   Growing from Center  /////
///////////////////////////////////
cout << " growing from center " << endl; 


    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
	 	  if (*(nim_landmarks_data  + nxy*iz + nx*ix  + iy  ) == 1 &&  abs((int) (*(nim_layers_data  + nxy*iz + nx*ix  + iy) - layernumber/2 )) < 2) {  //defining seed at center landmark
			*(growfromCenter_data  + nxy*iz + nx*ix  + iy  ) = 1.; 
			*(Grow_x_data  + nxy*iz + nx*ix  + iy  ) = ix ; 
			*(Grow_y_data  + nxy*iz + nx*ix  + iy  ) = iy ; 
			*(Grow_z_data  + nxy*iz + nx*ix  + iy  ) = iz ; 
		  }
        }
      }
    }


cout << " crowing "<<vinc_max <<  " iteration " << flush ; 

  for (int grow_i = 1 ; grow_i < vinc_max ; grow_i++ ){
		cout << grow_i <<  " " << flush ; 


    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){

	dist_min2 = 10000.;
	  x1g = 0;
	  y1g = 0;
	  z1g = 0;
	  
	  
	   if ( abs((int) (*(nim_layers_data  + nxy*iz + nx*ix  + iy) - layernumber/2 )) < 2 && *(growfromCenter_data  + nxy*iz + nx*ix  + iy  ) == 0  && *(nim_landmarks_data  + nxy*iz + nx*ix  + iy) < 2){
	   	// only grow into areas that are GM and that have not been gown into, yet .... and it should stop as soon as it hits tie border
	    	for(int iy_i=max(0,iy-grow_vinc_area); iy_i<min(iy+grow_vinc_area+1,sizePhase); ++iy_i){
	     	 for(int ix_i=max(0,ix-grow_vinc_area); ix_i<min(ix+grow_vinc_area+1,sizeRead); ++ix_i){
	     	  for(int iz_i=max(0,iz-grow_vinc_area); iz_i<min(iz+grow_vinc_area+1,sizeSlice); ++iz_i){
	     	  	dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i, dX, dY, dZ); 

	     	  
			  	if (*(growfromCenter_data  + nxy*iz_i + nx*ix_i  + iy_i  ) == grow_i   &&  *(nim_landmarks_data  + nxy*iz + nx*ix  + iy) < 2  ){
		 
			  
			  		if (dist_i < dist_min2 ){
			    		dist_min2 = dist_i ; 
			    		x1g = ix_i;
			    		y1g = iy_i;
			    		z1g = iz_i;
			    		dist_p1 = dist_min2; 

			  		}
			 	}   
			}  
	  	 }
	  	}
		if ( dist_min2 < 1.7 ){    /// ???? I DONT REMEMBER WHY I NEED THIS ????
			//distDebug(0,islice,iy,ix) = dist_min2 ; 
 	   	    *(growfromCenter_data  + nxy*iz + nx*ix  + iy  ) = grow_i+1 ;
			*(Grow_x_data  + nxy*iz + nx*ix  + iy  )  = *(Grow_x_data  + nxy*(int)z1g + nx*(int)x1g  + (int)y1g  ); 
			*(Grow_y_data  + nxy*iz + nx*ix  + iy  )  = *(Grow_y_data  + nxy*(int)z1g + nx*(int)x1g  + (int)y1g  ); 
			*(Grow_z_data  + nxy*iz + nx*ix  + iy  )  = *(Grow_z_data  + nxy*(int)z1g + nx*(int)x1g  + (int)y1g  );   
		}
		
		
		//cout << " ix   "  << ix << " iy   "  << iy  << "    " << *(WMkoord0_data  + nxy*islice + nx*(int)x1g  + (int)y1g  )<< endl; 
	   }
	   

        }
      }
    }
 }
cout << endl <<  " growing is done " << flush ; 




cout << " writing out " << endl; 

  const char  *fout_5="collumnar_distance.nii" ;
  if( nifti_set_filenames(growfromCenter, fout_5 , 1, 1) ) return 1;
  nifti_image_write( growfromCenter );



///////////////////////////////////////////
////   smooth columns  /////
//cout << " smooth columns " << endl; 
///////////////////////////////////////////

float gaus (float distance, float sigma) ;
 cout << " smoothing in middle layer   " << endl ; 

nifti_image * smoothed  	= nifti_copy_nim_info(nim_layers);
nifti_image * gausweight  	= nifti_copy_nim_info(nim_layers);
smoothed->datatype 		= NIFTI_TYPE_FLOAT32; 
gausweight->datatype 	= NIFTI_TYPE_FLOAT32;
smoothed->nbyper 		= sizeof(float);
gausweight->nbyper 		= sizeof(float);
smoothed->data = calloc(smoothed->nvox, smoothed->nbyper);
gausweight->data = calloc(gausweight->nvox, gausweight->nbyper);
float  *smoothed_data = (float *) smoothed->data;
float  *gausweight_data = (float *) gausweight->data;    


//float kernal_size = 10; // corresponds to one voxel sice. 
int  FWHM_val = 1 ; 
int vinc_sm =  max( (float)(1.) , (float)(2 * FWHM_val/dX )); // if voxel is too far away, I ignore it. 
dist_i = 0.;
cout << "       vinc_sm " <<  vinc_sm<<  endl; 
cout << "      FWHM_val " <<  FWHM_val<<  endl; 
//cout << " DEBUG " <<   dist(1.,1.,1.,1.,2.,1.,dX,dY,dZ) << endl; 
cout << "     here 2 " <<  endl; 


	for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
          *(gausweight_data  + nxy*iz + nx*ix  + iy  )  = 0 ; 
		  //  *(smoothed_data    + nxy*iz + nx*ix  + iy  )  = 0 ; 
		  
	     if (*(growfromCenter_data   +  nxy*iz + nx*ix  + iy  )  > 0 ){
		
			for(int iz_i=max(0,iz-vinc_sm); iz_i<min(iz+vinc_sm+1,sizeSlice); ++iz_i){
	    		for(int iy_i=max(0,iy-vinc_sm); iy_i<min(iy+vinc_sm+1,sizePhase); ++iy_i){
	      			for(int ix_i=max(0,ix-vinc_sm); ix_i<min(ix+vinc_sm+1,sizeRead); ++ix_i){
	      			  if (*(growfromCenter_data   +  nxy*iz_i + nx*ix_i  + iy_i  )  > 0 ){
		  				dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i,dX,dY,dZ); 
		  				//cout << "debug  4 " <<  gaus(dist_i ,FWHM_val ) <<   endl; 
		  			    //cout << "debug  5 " <<  dist_i  <<   endl; 
						//if ( *(nim_input_data   +  nxy*iz + nx*ix  + iy  )  == 3 ) cout << "debug  4b " << endl; 
						//dummy = *(layer_data  + nxy*iz_i + nx*ix_i  + iy_i  ); 
		  				*(smoothed_data    + nxy*iz + nx*ix  + iy  ) = *(smoothed_data    + nxy*iz + nx*ix  + iy  ) + *(growfromCenter_data  + nxy*iz_i + nx*ix_i  + iy_i  ) * gaus(dist_i ,FWHM_val ) ;
		    			*(gausweight_data  + nxy*iz + nx*ix  + iy  ) = *(gausweight_data  + nxy*iz + nx*ix  + iy  ) + gaus(dist_i ,FWHM_val ) ; 

			  		  }	
		            }	  
	      	    }
	       }
	       if (*(gausweight_data  + nxy*iz + nx*ix  + iy  ) > 0 ) *(smoothed_data    + nxy*iz + nx*ix  + iy  )  = *(smoothed_data    + nxy*iz + nx*ix  + iy  )/ *(gausweight_data  + nxy*iz + nx*ix  + iy  );
	     }
	     //if (*(nim_layers_r_data   +  nxy*iz + nx*ix  + iy  )  <= 0 )	     	*(smoothed_data    + nxy*iz + nx*ix  + iy  ) =  *(lateralCoord_data  + nxy*iz + nx*ix  + iy  ) ;
        }
      }
    }

cout << "     here 2 " <<  endl; 

 
    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead; ++ix){
           if(*(growfromCenter_data  + nxy*iz + nx*ix  + iy  ) > 0) {
			*(growfromCenter_data + nxy*iz + nx*ix  + iy  ) = (int) *(smoothed_data    + nxy*iz + nx*ix  + iy  )  ; 
		   }
        }
      }
    }

/////////////////////////////////////////////  This is not perfect yet, because it has only 4 directions to grow thus ther might be orientation biases.
////   extending columns across layers  /////
/////////////////////////////////////////////
cout << " extending columns across layers  " << endl; 


nifti_image * hairy_brain  	= nifti_copy_nim_info(nim_layers);
hairy_brain->datatype 		= NIFTI_TYPE_INT16; 
hairy_brain->nbyper 		= sizeof(short);
hairy_brain->data = calloc(hairy_brain->nvox, hairy_brain->nbyper);
short  *hairy_brain_data = (short *) hairy_brain->data;
nifti_image * hairy_brain_dist  	= nifti_copy_nim_info(nim_layers);
hairy_brain_dist->datatype 		= NIFTI_TYPE_FLOAT32; 
hairy_brain_dist->nbyper 		= sizeof(float);
hairy_brain_dist->data = calloc(hairy_brain_dist->nvox, hairy_brain_dist->nbyper);
float  *hairy_brain_dist_data = (float *) hairy_brain_dist->data;

dist_min2 = 10000. ; // this is an upper limit of the cortical thickness
int vinc_thickness = 13 ; // this is the area the algorithm looks for the closest middel elayer
int vinc_steps = 1 ; // this is step size that neigbouring GM voxels need to be to be classified as one side of the GM bank
int cloasest_coord = 0. ;
int there_is_close_noigbour = 0;
float average_neigbours = 0 ;
float average_val = 0 ;

cout << " crowing "<<vinc_thickness <<  " iteration aross layers " << flush ; 

for (int iiteration = 0 ; iiteration < vinc_thickness ; ++iiteration ){
cout << " "<<iiteration  << flush ; 

   for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){
		*(hairy_brain_data  + nxy*iz + nx*ix  + iy) =  0 ; 
      }
    }     
   }


   for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){

	  	
		if ( *(growfromCenter_data  + nxy*iz + nx*ix  + iy) > 0  ) {
		*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) =  *(growfromCenter_data  + nxy*iz + nx*ix  + iy) ; 
		
		  for(int   iz_i=max(0,iz-vinc_steps); iz_i<=min(iz+vinc_steps,sizeSlice); ++iz_i){
 	       for(int  iy_i=max(0,iy-vinc_steps); iy_i<=min(iy+vinc_steps,sizePhase); ++iy_i){
	     	for(int ix_i=max(0,ix-vinc_steps); ix_i<=min(ix+vinc_steps,sizeRead ); ++ix_i){
	     	   if ( *(growfromCenter_data  + nxy*iz_i + nx*ix_i  + iy_i) == 0  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) > 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) < layernumber-1 ) {
	     	    if (dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i,1,1,1) <= 1) {
	     	    	*(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) = *(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) + 1; 
	     	    }
	     	   }
	     	} 
		   } 		
		  }
		  
		
		 
	    } 

		 
	  } 
  	 } 		
	}
	
	
  for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){

	  	
		if ( *(growfromCenter_data  + nxy*iz + nx*ix  + iy) > 0  ) {
		//*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) =  *(growfromCenter_data  + nxy*iz + nx*ix  + iy) ; 
		
	
        for(int   iz_i=max(0,iz-vinc_steps); iz_i<=min(iz+vinc_steps,sizeSlice); ++iz_i){
 	       for(int  iy_i=max(0,iy-vinc_steps); iy_i<=min(iy+vinc_steps,sizePhase); ++iy_i){
	     	for(int ix_i=max(0,ix-vinc_steps); ix_i<=min(ix+vinc_steps,sizeRead ); ++ix_i){
	     	   if ( *(growfromCenter_data  + nxy*iz_i + nx*ix_i  + iy_i) == 0 && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) > 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) < layernumber-1 ) {
	     	   	if (dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i,1,1,1) <= 1) { 
	     	   		 *(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i) =  *(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i)+ *(growfromCenter_data  + nxy*iz + nx*ix  + iy) / *(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) ;
	     	    }
	     	   }
	     	} 
		   } 		
		  }	
		  
		  
		 
	    } 

		 
	  } 
  	 } 		
	}	
	
	
   for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){
		*(growfromCenter_data  + nxy*iz + nx*ix  + iy) =  *(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) ; 
      }
    }     
   }
	
		  
 } // GROWING itterations are done


cout << endl; 


///////////////////////////////////////////
////   smooth columns  /////
///////////////////////////////////////////

cout << " smoothing the thick cortex now  " << endl; 

   for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
		  *(smoothed_data    + nxy*iz + nx*ix  + iy  )  = 0 ; 
        }
      }
    }

FWHM_val = 3 ;
vinc_sm = 8 ; // max(1.,2. * FWHM_val/dX ); // This is small so that neighboring sulcu are not affecting each other
cout << "      vinc_sm " <<  vinc_sm<<  endl; 
cout << "      FWHM_val " <<  FWHM_val<<  endl; 

cout << "  starting extended now  " <<  endl; 

	for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
          *(gausweight_data  + nxy*iz + nx*ix  + iy  )  = 0 ; 
		  //  *(smoothed_data    + nxy*iz + nx*ix  + iy  )  = 0 ; 
		  
	      if (*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) > 0 ){
	      
	      ///////////////////////////////////////////////////////
	      // find area that is not from the other sulcus 
	      //////////////////////////////////////////////////////
	      
	      //PREPARATEION OF DUMMY VINSINITY FILE
	      
	      	 for(int iz_i=max(0,iz-vinc_sm-vinc_steps); iz_i<=min(iz+vinc_sm+vinc_steps,sizeSlice); ++iz_i){
	    		for(int iy_i=max(0,iy-vinc_sm-vinc_steps); iy_i<=min(iy+vinc_sm+vinc_steps,sizePhase); ++iy_i){
	      			for(int ix_i=max(0,ix-vinc_sm-vinc_steps); ix_i<=min(ix+vinc_sm+vinc_steps,sizeRead); ++ix_i){	      			  
	      				*(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) = 0 ;	      			  
	      	       }
	      	    }
	          }	
	      *(hairy_brain_data  + nxy*iz + nx*ix  + iy) = 1 ;
	      for (int K_ = 0 ; K_ < vinc_sm ; K_++){
	      
	       for(int iz_ii=max(0,iz-vinc_steps); iz_ii<=min(iz+vinc_steps,sizeSlice); ++iz_ii){
	    		for(int iy_ii=max(0,iy-vinc_steps); iy_ii<=min(iy+vinc_steps,sizePhase); ++iy_ii){
	      			for(int ix_ii=max(0,ix-vinc_steps); ix_ii<=min(ix+vinc_steps,sizeRead); ++ix_ii){
					      if (*(hairy_brain_data  + nxy*iz_ii + nx*ix_ii  + iy_ii) == 1 ) {
					       for(int iz_i=max(0,iz_ii-vinc_steps); iz_i<=min(iz_ii+vinc_steps,sizeSlice); ++iz_i){
					    		for(int iy_i=max(0,iy_ii-vinc_steps); iy_i<=min(iy_ii+vinc_steps,sizePhase); ++iy_i){
					      			for(int ix_i=max(0,ix_ii-vinc_steps); ix_i<=min(ix_ii+vinc_steps,sizeRead); ++ix_i){
					      			  if (dist((float)ix_ii,(float)iy_ii,(float)iz_ii,(float)ix_i,(float)iy_i,(float)iz_i,1,1,1) <= 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) > 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) < layernumber-1) { 
					      				*(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) = 1 ; 
					      			  }	
				 	      	        }
					      	    }
				           }	
		  	              }     
    	             }
	      	      }
	           }	    
	      }
	      
	      
		    int layernumber_i =  *(nim_layers_data  +  nxy*iz + nx*ix  + iy ) ; 
		  
			   for(int iz_i=max(0,iz-vinc_sm); iz_i<=min(iz+vinc_sm,sizeSlice); ++iz_i){
	    		for(int iy_i=max(0,iy-vinc_sm); iy_i<=min(iy+vinc_sm,sizePhase); ++iy_i){
	      			for(int ix_i=max(0,ix-vinc_sm); ix_i<=min(ix+vinc_sm,sizeRead); ++ix_i){
	      			  if ( *(hairy_brain_data  + nxy*iz_i + nx*ix_i  + iy_i) == 1 && abs((int) *(nim_layers_data   +  nxy*iz_i + nx*ix_i  + iy_i  ) - layernumber_i ) < 2 && *(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i) > 0 ){
		  				dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i,dX,dY,dZ); 
		  				//cout << "debug  4 " <<  gaus(dist_i ,FWHM_val ) <<   endl; 
		  			    //cout << "debug  5 " <<  dist_i  <<   endl; 
						//if ( *(nim_input_data   +  nxy*iz + nx*ix  + iy  )  == 3 ) cout << "debug  4b " << endl; 
						//dummy = *(layer_data  + nxy*iz_i + nx*ix_i  + iy_i  ); 
		  				*(smoothed_data    + nxy*iz + nx*ix  + iy  ) = *(smoothed_data    + nxy*iz + nx*ix  + iy  ) + *(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i) * gaus(dist_i ,FWHM_val ) ;
		    			*(gausweight_data  + nxy*iz + nx*ix  + iy  ) = *(gausweight_data  + nxy*iz + nx*ix  + iy  ) + gaus(dist_i ,FWHM_val ) ; 

			  		  }	
		            }	  
	      	    }
	          }
	       if (*(gausweight_data  + nxy*iz + nx*ix  + iy  ) > 0 ) *(smoothed_data    + nxy*iz + nx*ix  + iy  )  = *(smoothed_data    + nxy*iz + nx*ix  + iy  )/ *(gausweight_data  + nxy*iz + nx*ix  + iy  );
	     }
        }
      }
    }

cout << "  extended now  " <<  endl; 

 
    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead; ++ix){
           if(*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) > 0) {
			*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy) =  *(smoothed_data    + nxy*iz + nx*ix  + iy  )  ; 
		   }
        }
      }
    }


 // const char  *fout_8="smoothed_thick_columns.nii" ;
 // if( nifti_set_filenames(smoothed, fout_8 , 1, 1) ) return 1;
 // nifti_image_write( smoothed );


 	
/*
   for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){
		*(hairy_brain_dist_data  + nxy*iz + nx*ix  + iy) =  0 ; 
      }
    }     
   }

// *(nim_layers_data  + nxy*iz + nx*ix  + iy) > 0

   for(int iz=0; iz<sizeSlice; ++iz){  
    for(int iy=0; iy<sizePhase; ++iy){
      for(int ix=0; ix<sizeRead; ++ix){
	  	dist_min2 = 10000.;
	  	
		if ( *(growfromCenter_data  + nxy*iz + nx*ix  + iy) > 0 ) {
		
		  for(int   iz_i=max(0,iz-vinc_thickness); iz_i<=min(iz+vinc_thickness,sizeSlice); ++iz_i){
 	       for(int  iy_i=max(0,iy-vinc_thickness); iy_i<=min(iy+vinc_thickness,sizePhase); ++iy_i){
	     	for(int ix_i=max(0,ix-vinc_thickness); ix_i<=min(ix+vinc_thickness,sizeRead ); ++ix_i){
	     		*(hairy_brain_dist_data  + nxy*iz_i + nx*ix_i  + iy_i) =  0 ;
	     	} 
		   } 		
		  }
		  *(hairy_brain_dist_data  + nxy*iz + nx*ix  + iy) =  1 ;
		  
		  for (int iiteration = 0 ; iiteration < vinc_thickness*2 ; ++ iiteration){
	         for(int   iz_i=max(0,iz-vinc_thickness); iz_i<=min(iz+vinc_thickness,sizeSlice); ++iz_i){
 	          for(int  iy_i=max(0,iy-vinc_thickness); iy_i<=min(iy+vinc_thickness,sizePhase); ++iy_i){
	     	   for(int ix_i=max(0,ix-vinc_thickness); ix_i<=min(ix+vinc_thickness,sizeRead ); ++ix_i){
	     	   
	     	      if (*(hairy_brain_dist_data  + nxy*iz_i + nx*ix_i  + iy_i)  == 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) > 1  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) < layernumber-1){		 		
	     	  		 for(int   iz_ii=max(0,iz_i-vinc_steps); iz_ii<=min(iz_i+vinc_steps,sizeSlice); ++iz_ii){
 	           			for(int  iy_ii=max(0,iy_i-vinc_steps); iy_ii<=min(iy_i+vinc_steps,sizePhase); ++iy_ii){
	     	    			for(int ix_ii=max(0,ix_i-vinc_steps); ix_ii<=min(ix_i+vinc_steps,sizeRead ); ++ix_ii){
	     	    			     if ( *(nim_layers_data  + nxy*iz_ii + nx*ix_ii  + iy_ii) > 1  && *(nim_layers_data  + nxy*iz_ii + nx*ix_ii  + iy_ii) < layernumber-1){
			    					*(hairy_brain_dist_data  + nxy*iz_ii + nx*ix_ii  + iy_ii)  = 1  ;
			    				 }	
							}	
						}
					 }		
				   }	
				   

	  	 	   }
	  	      } 
	  	     }
		  }

          dist_min2 = 10000. ;
      	  for(int   iz_i=max(0,iz-vinc_thickness); iz_i<=min(iz+vinc_thickness,sizeSlice); ++iz_i){
 	       for(int  iy_i=max(0,iy-vinc_thickness); iy_i<=min(iy+vinc_thickness,sizePhase); ++iy_i){
	     	for(int ix_i=max(0,ix-vinc_thickness); ix_i<=min(ix+vinc_thickness,sizeRead ); ++ix_i){
	     	
	     	  if (*(hairy_brain_dist_data  + nxy*iz_i + nx*ix_i  + iy_i)  == 1){
	     	   dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i,dX,dY,dZ); 
	     	     if ( dist_i < dist_min2 ) {
	     		  *(growfromCenter_data  + nxy*iz_i + nx*ix_i  + iy_i) =  *(growfromCenter_data  + nxy*iz + nx*ix  + iy) ;
	     		  dist_min2 = dist_i ; 
	     		 } 
	     	  }
	     	} 
		   } 		
		  }

	   }
      }
    }     
   }

 // const char  *fout_7="collumnar_distance_thick.nii" ;
 // if( nifti_set_filenames(growfromCenter, fout_7 , 1, 1) ) return 1;
 // nifti_image_write( growfromCenter );

 // const char  *fout_7="thick_ribbon.nii" ;
 // if( nifti_set_filenames(hairy_brain, fout_7 , 1, 1) ) return 1;
  //nifti_image_write( hairy_brain );


//  const char  *fout_8="thick_ribbon_smoothed.nii" ;
//  if( nifti_set_filenames(hairy_brain, fout_8 , 1, 1) ) return 1;
//  nifti_image_write( hairy_brain );

*/

/*
///////////////////////////////////
////   Growing from as thick cortex  /////
///////////////////////////////////
cout << " growing from center with thick cortex " << endl; 

int grow_vinc_thick = 1 ;
int vinc_max_thick = 17 ; 
int grow_vinc_area_thick = 1 ;



    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
	 	  if (*(growfromCenter_data  + nxy*iz + nx*ix  + iy  ) > 0) {  //defining seed at center landmark
			*(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy  ) = 1; 
		  }
        }
      }
    }


  for (int grow_i = 1 ; grow_i < vinc_max_thick ; grow_i++ ){
    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){


	  
	   if (  *(nim_layers_data  + nxy*iz + nx*ix  + iy) > 0 && *(nim_layers_data  + nxy*iz + nx*ix  + iy) < layernumber && *(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy  ) == grow_i && *(hairy_brain_data  + nxy*iz + nx*ix  + iy  ) > 0){
	   	// only grow into areas that are GM and that have not been gown into, yet .... and it should stop as soon as it hits tie border
	    	int iz_i=iz ; 
	    	//int stopme = 0 ; 
	    	
	     	 for(int ix_i=max(0,ix-grow_vinc_area_thick); ix_i<min(ix+grow_vinc_area_thick+1,sizeRead); ++ix_i){
	     	  for(int iy_i=max(0,iy-grow_vinc_area_thick); iy_i<min(iy+grow_vinc_area_thick+1,sizePhase); ++iy_i){
	     	    dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i, dX, dY, dZ); 
				//if ( *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) == layernumber ) stopme = 1;
			  	if (dist_i <= (dY+dX)/2. && *(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i  ) == 0   && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) < layernumber  && *(nim_layers_data  + nxy*iz_i + nx*ix_i  + iy_i) >0 && *(hairy_brain_data  + nxy*iz + nx*ix  + iy  ) > 0 ){
			  		*(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i  ) = grow_i+1 ;
			 	}   
			}  
	  	 
	  	}
		
		
		
		//cout << " ix   "  << ix << " iy   "  << iy  << "    " << *(WMkoord0_data  + nxy*islice + nx*(int)x1g  + (int)y1g  )<< endl; 
	   }
	   

        }
      }
    }
 }


    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){  
	      if ( ( *(nim_layers_data  + nxy*iz + nx*ix  + iy) == layernumber-1 || *(nim_layers_data  + nxy*iz + nx*ix  + iy) == layernumber-2 ) && *(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy  ) > 0 ){
	   	// only grow into areas that are GM and that have not been gown into, yet .... and it should stop as soon as it hits tie border
	    	for(int iy_i=max(0,iy-grow_vinc_area_thick); iy_i<min(iy+grow_vinc_area_thick+1,sizePhase); ++iy_i){
	     	 for(int ix_i=max(0,ix-grow_vinc_area_thick); ix_i<min(ix+grow_vinc_area_thick+1,sizeRead); ++ix_i){
	     	  int iz_i=iz;
	     	  dist_i = dist((float)ix,(float)iy,(float)iz,(float)ix_i,(float)iy_i,(float)iz_i, dX, dY, dZ); 

			  	if ( dist_i <= (dY+dX)/2. &&  *(nim_landmarks_data  + nxy*iz_i + nx*ix_i  + iy_i) > 0 ){
			  		*(growfromCenter_thick_data  + nxy*iz_i + nx*ix_i  + iy_i  ) = *(growfromCenter_thick_data  + nxy*iz + nx*ix  + iy  ) ;
			 	}   
			  
	  	     }
	  	    }
	     }
        }
      }
    }
*/

 // const char  *fout_6="growfromCenter_thick.nii" ;
 // if( nifti_set_filenames(growfromCenter_thick, fout_6 , 1, 1) ) return 1;
 // nifti_image_write( growfromCenter_thick );
  
 


///////////////////////////////////////////
////         clean up hairy brain  /////
///////////////////////////////////////////
cout << " clean up hairy brain " << endl; 
/*
    for(int iz=0; iz<sizeSlice; ++iz){  
      for(int iy=0; iy<sizePhase; ++iy){
        for(int ix=0; ix<sizeRead-0; ++ix){
           if( *(growfromCenter_thick_data + nxy*iz + nx*ix  + iy  ) == 0 || *(hairy_brain_data  + nxy*iz + nx*ix  + iy  ) == 0) {
		     *(hairy_brain_data  + nxy*iz + nx*ix  + iy  ) = 0 ;
			}
       }
      }
    }
*/
  const char  *fout_3="hairy_brain.nii" ;
  if( nifti_set_filenames(hairy_brain, fout_3 , 1, 1) ) return 1;
  nifti_image_write( hairy_brain );


  const char  *fout_2="growfromCenter_thick.nii" ;
  if( nifti_set_filenames(growfromCenter_thick, fout_2 , 1, 1) ) return 1;
  nifti_image_write( growfromCenter_thick );






  return 0;
}




  float dist (float x1, float y1, float z1, float x2, float y2, float z2, float dX, float dY, float dZ ) {
    return sqrt((x1-x2)*(x1-x2)*dX*dX+(y1-y2)*(y1-y2)*dY*dY+(z1-z2)*(z1-z2)*dZ*dZ);
  }


  float gaus (float distance, float sigma) {
    return 1./(sigma*sqrt(2.*3.141592))*exp (-0.5*distance*distance/(sigma*sigma));
  }

