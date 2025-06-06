/********************************************************
 *
 * EYELINK PORTABLE EXPT SUPPORT
 * Copyright (c) 1996-2021 SR Research Ltd.
 *
 * 27 September 2002 by Suganthan Subramaniam
 * For non-commercial use only
 *
 * EyeLink library data extensions
 *        VERSION 2.5
 * UPDATED for EyeLink DLL V2.1: added LOST_DATA_EVENT
 * This module is for user applications
 * Use is granted for non-commercial
 * applications by Eyelink licencees only
 *
 * UNDER NO CIRCUMSTANCES SHOULD PARTS OF THESE FILES
 * BE COPIED OR COMBINED.  This will make your code
 * impossible to upgrade to new releases in the future,
 * and SR Research will not give tech support for
 * reorganized code.
 *
 * This file should not be modified. If you must modify it,
 * copy the entire file with a new name, and change the
 * the new file.
 *
 * EDFTYPES.H has platform-portable definitions COPYRIGHT
 * TERMS:  This file is part of the EyeLink application
 * support package.  You may compile this code, and may
 * read it to understand the EyeLink data structures.
 *
 *
 * DO NOT MODIFY THIS CODE IN ANY WAY.
 * SR Research will not support any problems caused by
 * modifying this code, and using modified code will make
 * your programs incompatible with future releases.
 *
 * This code has been tested on many 16 and
 * 32-bit compilers, including Borland and
 * Microsoft, GCC, in Windows, MAC OS X, Linux and DOS.
 * It should  compile without warnings on most platforms
 * If you must port to a new platform, check
 * the data types defined in EDFTYPES.H for
 * correct sizes.
 *
 *
 *
 ***********************************************************/



#include <stdio.h>
#include <math.h>
#include <assert.h>
#ifdef __APPLE__
#include <edfapi/edf.h>
#else
#include <edf.h>
#endif
#include "opt.h"


/***********************************************************************
 *Global variable externs
 ***********************************************************************/
extern Opt options;
double ppd_x =0;
double ppd_y=0;
double ppd_count=0;



/***********************************************************************
 *Global function externs
 ***********************************************************************/
extern void print_value(double val, int dp);


/***********************************************************************
 *Function Name: get_sample_data
 *Input: sample, pointers to x, y, ave, and eye
 *Output: returns x, y, avg
 *Purpose: get the appropriate value from the sample.
 ***********************************************************************/
int get_sample_data(FSAMPLE * sam, float *xp, float *yp,  float *ap, int eye,int samtype)
{
  float x = NaN;
  float y = NaN;
  float a = 0;
  int found = 0;

  if((sam->flags & (eye?SAMPLE_RIGHT:SAMPLE_LEFT)))
    {
      if(!options.disable_pa_check && sam->flags & SAMPLE_PUPILSIZE)
	{
	  a = sam->pa[eye];
	  if(a<15) goto out;
	}
      if(samtype==OUTPUT_GAZE)
	{
	  if(sam->flags&SAMPLE_GAZEXY)
	    {
	        x = (sam->gx[eye]);
	        y = (sam->gy[eye]);
	    }
	  else x = y = NaN;

	  if ( x==NaN || y ==NaN)   //api-239
	  	  a = 0;

	}
      else if(samtype==OUTPUT_HREF)
	{
	  if(sam->flags&SAMPLE_HREFXY)
	    {
	      x = sam->hx[eye];
	      y = sam->hy[eye];
	    }
	  else x = y = NaN;
	}
      else if(samtype==OUTPUT_PUPIL)
	{
	  if(sam->flags&SAMPLE_PUPILXY)
	    {
	      x = sam->px[eye];
	      y = sam->py[eye];
	    }
	  else x = y = NaN;
	}
      found = 1;
    }
 out:
  if(xp) *xp = x;
  if(yp) *yp = y;
  if(ap) *ap = a;
  return found;
}



/***********************************************************************
 *Function Name: updatePPD
 *Input: sample
 *Output: none
 *Purpose: updates the ppd namely the variables ppd_x, ppd_y, and
 *		   ppd_count
 ***********************************************************************/
void updatePPD(FSAMPLE * s)
{
  float a;
  int hasleft = (s->flags & SAMPLE_LEFT)!=0;
  int hasright = (s->flags & SAMPLE_RIGHT)!=0;

  if(options.output_sample_type==OUTPUT_HREF)
    {	  
      float x, y;
      float xracc = 0;
      float yracc = 0;
      float xrc = 0;
      float yrc = 0;

		
      if(options.out_sample_left && hasleft)
	  {
		get_sample_data(s,&x, &y, &a, 0,OUTPUT_HREF);
		if(x!=NaN)
	    {
	      xracc += x;
	      xrc++;
	    }
		if(y!=NaN)
	    {
	      yracc += y;
	      yrc++;
	    }
	  }
      if(options.out_sample_right && hasright)
	  {
		  get_sample_data(s,&x, &y, &a, 1,OUTPUT_HREF);
		  if(x!=NaN)
		  {
			xracc += x;
			xrc++;
		  }
		  if(y!=NaN)
		  {
			  yracc += y;
			  yrc++;
		  }
	  }
      if(xrc && yrc)
	  {
		float x1 = xracc/xrc;
		float y1 = yracc/yrc;
		s->rx = (float)((15000.0*15000.0+x1*x1+y1*y1)/
			sqrt(15000.0*15000.0+y1*y1) / 57.2958);
		s->ry = (float)((15000.0*15000.0+x1*x1+y1*y1)/
			sqrt(15000.0*15000.0+x1*x1) / 57.2958);
	  }
  }
  else if(options.output_sample_type==OUTPUT_GAZE)
  {
	  if(!(s->flags & SAMPLE_GAZERES))
	  {
		  s->rx = options.default_resolution_x;
		  s->ry = options.default_resolution_y;	  
	  }
  }
  else if(options.output_sample_type==OUTPUT_PUPIL)
  {
	  s->rx = options.default_resolution_x;
	  s->ry = options.default_resolution_y;
  }

  if(s->rx>0.01 && s->rx<10000 && s->ry>0.01 && s->ry<10000 )
  {
	  ppd_x += s->rx;
      ppd_y += s->ry;
      ppd_count++;
  }
}




/***********************************************************************
 *Function Name: write_sample
 *Input:		 sample, and edffile structure
 *Output: int
 *Purpose: writes the sample in readable asc format
 ***********************************************************************/
int write_raw_sample(EDFFILE * combfile, RECORDINGS * rec, FSAMPLE *s);
int write_sample(EDFFILE * combfile, RECORDINGS * rec, FSAMPLE *s)
{
  int hasleft =  ((rec->eye == 1 || rec->eye == 3)) && ((s->flags & SAMPLE_LEFT)!=0); // bugfix API-416
  int hasright = ((rec->eye == 2 || rec->eye == 3)) && ((s->flags & SAMPLE_RIGHT)!=0);
  float xracc = 0;
  float yracc = 0;

//  float g[2][2]={{692.53,747.23},{708.78,402.14}};
//  float h[2][2]={{1433.31, 3159.58},{1615.55, 488.24}};

  float xl, yl, al, xr, yr, ar; //


  /*s->gx[0]=693.3;
  s->gy[0]=746.24;
  s->hx[0]=1439.96;
  s->hy[0]=3152.12;

  s->gx[1]=708.79;
  s->gy[1]=402.36;
  s->hx[1]=1615.60;
  s->hy[1]=490.00;
*/


  if(!options.samples_enabled) return 1;

  if(options.allow_raw == -1 || options.allow_raw == 1)
  {
	  if((hasleft && s->pa[0]<0) ||(hasright && s->pa[1]<0))
	  {
		options.allow_raw = 1;
		write_raw_sample(combfile, rec, s);
		goto targetdata;
	  }
  }
  if(options.allow_raw == -1) 
	  options.allow_raw = 0; // cannot swap back and forth


  updatePPD(s);
  if(options.out_float_time)
	  print("%-8.1f", (float)(s->time+((s->flags & SAMPLE_ADD_OFFSET)?0.5:0)));
  else
	print("%u", s->time);      //Note: %lu --> %u for unsigned int


  if(options.output_left_eye && options.out_sample_left && hasleft)
    {
      //float x, y, a;
      get_sample_data(s,&xl, &yl, &al, 0,options.output_sample_type);  //api-239
      print_value(xl,1);
      print_value(yl,1);
	  print_value(al,1);
    }
  if(options.output_right_eye && options.out_sample_right && hasright)
    {
      //float x, y, a;
      get_sample_data(s,&xr, &yr, &ar, 1,options.output_sample_type); //api-239
      print_value(xr,1);
      print_value(yr,1);
      print_value(ar,1);
  }

   /* if(options.output_left_eye && options.out_sample_left && hasleft)
    {
      //float x, y, a;
      get_sample_data(s,&xl, &yl, &al, 0,OUTPUT_HREF);  //api-239
      print_value(xl,1);
      print_value(yl,1);
	  print_value(al,1);
    }
  if(options.output_right_eye && options.out_sample_right && hasright)
    {
      //float x, y, a;
      get_sample_data(s,&xr, &yr, &ar, 1,OUTPUT_HREF); //api-239
      print_value(xr,1);
      print_value(yr,1);
      print_value(ar,1);
  }*/
  
	 
  if(hasleft && hasright)
  {
	  if(options.sepres)
	  {

		if(options.sepres ==1)
		{
		
			edf_compute_physical_setup(options.simulation_screen_distance,options.simulation_screen_distance_bot,
				options.screen_phys_l, options.screen_phys_t, options.screen_phys_r, options.screen_phys_b,
				options.screen_pixel_l,options.screen_pixel_t,options.screen_pixel_r,options.screen_pixel_b
				);
			options.sepres = 2;
		}

		
		if(hasleft)
		{
			double rx=0,ry=0;
			edf_calculate_ppd(options.screen_pixel_r-options.screen_pixel_l,options.screen_pixel_b-options.screen_pixel_t,s,0,&rx, &ry);
			print_value(rx,2);
			print_value(ry,2);
		}
		if(hasright)
		{
			double rx=0,ry=0;
			edf_calculate_ppd(options.screen_pixel_r-options.screen_pixel_l,options.screen_pixel_b-options.screen_pixel_t,s,1,&rx, &ry);
			print_value(rx,2);
			print_value(ry,2);
		}
	  }
    }


  if(options.output_sample_velocity)
  {
	float xlvel=NaN, xrvel=NaN, ylvel=NaN, yrvel=NaN;
    if(options.fast_velocity)
    {
        switch(options.output_sample_type)
	    {
	    case OUTPUT_GAZE:
			if(xl!= NaN)   //api-239 same for below, all NaN check
		    xlvel = s->fgxvel[0];
			if(xr!= NaN)
		    xrvel = s->fgxvel[1];
			if(yl!= NaN)
		    ylvel = s->fgyvel[0];
			if(yr!= NaN)
		    yrvel = s->fgyvel[1];
		    break;
	    case OUTPUT_HREF:
			if(xl!= NaN)
		    xlvel = s->fhxvel[0];
			if(xr!= NaN)
		    xrvel = s->fhxvel[1];
			if(yl!= NaN)
		    ylvel = s->fhyvel[0];
			if(yr!= NaN)
		    yrvel = s->fhyvel[1];
            break;
	    case OUTPUT_PUPIL:
			if(xl!= NaN)
		    xlvel = s->frxvel[0];
			if(xr!= NaN)
		    xrvel = s->frxvel[1];
			if(yl!= NaN)
		    ylvel = s->fryvel[0];
			if(yr!= NaN)
		    yrvel = s->fryvel[1];
		    break;
	    default:
		    break;
	    }
    }
    else
    {
	    switch(options.output_sample_type)
	    {
	    case OUTPUT_GAZE:
			if(xl!= NaN)
		    xlvel = s->gxvel[0];
			if(xr!= NaN)
		    xrvel = s->gxvel[1];
			if(yl!= NaN)
		    ylvel = s->gyvel[0];
			if(yr!= NaN)
		    yrvel = s->gyvel[1];
		    break;
	    case OUTPUT_HREF:
			if(xl!= NaN)
		    xlvel = s->hxvel[0];
			if(xr!= NaN)
		    xrvel = s->hxvel[1];
			if(yl!= NaN)
		    ylvel = s->hyvel[0];
			if(yr!= NaN)
		    yrvel = s->hyvel[1];
            break;
	    case OUTPUT_PUPIL:
			if(xl!= NaN)
		    xlvel = s->rxvel[0];
			if(xr!= NaN)
		    xrvel = s->rxvel[1];
			if(yl!= NaN)
		    ylvel = s->ryvel[0];
			if(yr!= NaN)
		    yrvel = s->ryvel[1];
		    break;
	    default:
		    break;
	    }
    }
	if(options.output_left_eye &&  options.out_sample_left && hasleft)
	{
	  print_value(xlvel, 30); //REV: changed all these to 30...
	  print_value(ylvel, 30);
	}

	if(options.output_right_eye &&  options.out_sample_right && hasright)
	{
		print_value(xrvel, 30);
		print_value(yrvel, 30);
	}
  }




  if(options.output_resolution || options.output_sample_velocity) /* Need resolution? */
    {    					  /* already computed in output_sample */
      if ((s->rx>0.01 && s->rx<10000 &&s->ry>0.01 && s->ry<10000 ))
		{
			xracc = s->rx;
			yracc = s->ry;
		}
      else
		{
			xracc = options.default_resolution_x;
			yracc = options.default_resolution_y;
		}
    }
  else
    {
      xracc = options.default_resolution_x;
      yracc = options.default_resolution_y;
    }

    if(options.output_resolution)
    {
      print_value(xracc,2);
      print_value(yracc,2);
    }
    if(options.out_marker_fields==1)
      {
          if(s->flags & SAMPLE_HEADPOS)
          {

              int j =0;
              int i =s->htype;
              i &= 15;
              if(i>8) i = 8;
              if(i!=8) i =0;  /* if there is less than 8 markers, we put no values at the moment. */

              for(j =0; j <i; j++)
              {
                  double v = (double)(s->hdata[j]);
                  print_value(v,0);
              }
              for(j =i; j <8; j++)
              {
                  print_value(NaN,0);
              }
          }
		  
      }

  if(options.output_input_values)
  {
      print_value((double)(s->input),1) ;
  }

  if(options.output_button_values)
  {
      print_value((double)(s->buttons),1) ;
  }
		 /* NEW EYELINK II flags (interp/CR state)*/
  if(options.out_sample_flags && edf_get_revision(combfile)>0 && (s->flags&SAMPLE_STATUS))
    {
      print("\t");
      if(s->errors & INTERP_SAMPLE_WARNING)
		print("I"); else print(".");

      if(options.output_left_eye && options.out_sample_left && rec->recording_mode && hasleft)
		{
			print((s->errors&CR_LOST_LEFT_WARNING)  ? "C":".");
			print((s->errors&CR_RECOV_LEFT_WARNING) ? "R":".");
		}
      if(options.output_right_eye && options.out_sample_right && rec->recording_mode && hasright)
		{
			print((s->errors&CR_LOST_RIGHT_WARNING)  ? "C":".");
			print((s->errors&CR_RECOV_RIGHT_WARNING) ? "R":".");
		}
    }

  if(options.output_sample_type == OUTPUT_GAZE && hasleft && hasright && options.out_averages)
  {
      float xl =NaN,
            yl =NaN;
      float xr=NaN,
            yr=NaN;
      get_sample_data(s,&xl, &yl, NULL, 0,options.output_sample_type);
      get_sample_data(s,&xr, &yr,NULL, 1,options.output_sample_type);
      if(xl == NaN  ||yl == NaN  ||xr == NaN  ||yr == NaN)
      {
        print_value(NaN,1);
        print_value(NaN,1);
      }
      else
      {
        print_value((xl+xr)/2.0,1);
        print_value((yl+yr)/2.0,1);
      }
  }

  if(options.output_elcl)
  {
	  int eye =hasleft?0:1;
	  for(; eye<(hasright?2:1); eye++)
	  {
		float raw_pupil[2];
		float raw_cr[2];
		unsigned int pupil_area;
		unsigned int cr_area;
		UINT32 pupil_dimension[2];
		UINT32 cr_dimension[2];
		UINT32 window_position[2];
		float pupil_cr[2];
		

		edf_get_uncorrected_raw_pupil(combfile,s, eye,raw_pupil);
		edf_get_uncorrected_raw_cr(combfile,s, eye, raw_cr);
		pupil_area = edf_get_uncorrected_pupil_area(combfile,s, eye);
		cr_area = edf_get_uncorrected_cr_area(combfile,s, eye);
		edf_get_pupil_dimension(combfile,s, eye,pupil_dimension);
		edf_get_cr_dimension(combfile,s, cr_dimension);
		edf_get_window_position(combfile,s, window_position);
		edf_get_pupil_cr(combfile,s,eye, pupil_cr);

		//Note: %lu --> %u for unsigned int
		print("\t%8.3f\t%8.3f\t%8.3f\t%8.3f\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%8.3f\t%8.3f",
				raw_pupil[0],
				raw_pupil[1],
				raw_cr[0],
				raw_cr[1],
				pupil_area,
				cr_area,
				pupil_dimension[0], pupil_dimension[1],
				cr_dimension[0],    cr_dimension[1],
				window_position[0], window_position[1],
				pupil_cr[0],
				pupil_cr[1]);

	  }
  }
targetdata:
  if(options.enable_htarget  && options.out_marker_fields==2&&s->flags & SAMPLE_HEADPOS && (s->htype == 0xB6 || s->htype == 0xB7))
  {
	INT16 v = s->hdata[3];
	INT16 v1 = s->hdata[6];

	print(" ");
	print_value((s->hdata[0]==(INT16)0x8000)?NaN:s->hdata[0],1); //x
	print_value((s->hdata[1]==(INT16)0x8000)?NaN:s->hdata[1],1); //y
	print_value((s->hdata[2]==(INT16)0x8000)?NaN:(((double)s->hdata[2])/10.0),1); //distance

	if(options.out_sample_flags)
	{
		print(" ");
		print("%s",(v & TFLAG_MISSING) ? "M":".");  
		print("%s",(v & TFLAG_ANGLE)   ? "A":".");
		print("%s",(v & TFLAG_NEAREYE) ? "N":".");
		print("%s",(v & TFLAG_CLOSE)   ? "C":".");
		print("%s",(v & TFLAG_FAR)     ? "F":".");
		print("%s",(v & TFLAG_T_TSIDE) ? "T":".");
		print("%s",(v & TFLAG_T_BSIDE) ? "B":".");
		print("%s",(v & TFLAG_T_LSIDE) ? "L":".");
		print("%s",(v & TFLAG_T_RSIDE) ? "R":".");
		print("%s",(v & TFLAG_E_TSIDE) ? "T":".");
		print("%s",(v & TFLAG_E_BSIDE) ? "B":".");
		print("%s",(v & TFLAG_E_LSIDE) ? "L":".");
		print("%s",(v & TFLAG_E_RSIDE) ? "R":".");
		if(hasleft && hasright && s->htype == 0xB7)
		{
			print("%s",(v1 & TFLAG_E_TSIDE) ? "T":".");
			print("%s",(v1 & TFLAG_E_BSIDE) ? "B":".");
			print("%s",(v1 & TFLAG_E_LSIDE) ? "L":".");
			print("%s",(v1 & TFLAG_E_RSIDE) ? "R":".");
		}
	}
	
  }
  print("\n");
  return 0;
}

extern char missing_string[];
void print_value3(double val, int dp)
{
  if(val==NaN)
    {
	  print("\t%s", missing_string);
	  return;
    }

  if(dp==0)
    print("\t%7.0f", val);
  else
    {
      float am =(float) fabs(val);
      print("\t%7.*f", dp, val);
    }
}

int write_raw_sample(EDFFILE * combfile, RECORDINGS * rec, FSAMPLE *s)
{
  static int rawprint =0;
  int hasleft = (s->flags & SAMPLE_LEFT)!=0;
  int hasright = (s->flags & SAMPLE_RIGHT)!=0;
  int eye =0;

  if(rawprint ==0)
  {
	  rawprint = 1;
	  //print("LABELS PUP.X\tPUP.Y\tPUP.A\tPUP.W\tPUP.H\tCR.X\tCR.Y\tCR.A\tPCR.X\tPCR.Y\n");
	  print("LABELS:");
	  if(hasleft)
	  {
		  print("LPUP.X\tLPUP.Y\tLPUP.A\tLCR.X\tLCR.Y\tLCR.A\tLPCR.X\tLPCR.Y");
		  if((s->flags & SAMPLE_HEADPOS) && (s->htype==0xC4 || s->htype==0xC8))
			print("\tLCR2.X\tLCR2.Y\tLCR2.A");
	  }
	  if(hasright)
	  {
		print("\tRPUP.X\tRPUP.Y\tRPUP.A\tRCR.X\tRCR.Y\tRCR.A\tRPCR.X\tRPCR.Y");
		if((s->flags & SAMPLE_HEADPOS) && (s->htype==0xC4 || s->htype==0xC8))
			print("\tRCR2.X\tRCR2.Y\tRCR2.A");
	  }
	  print("\n");

  }
  if(options.out_float_time)
	print("%-8.1f", (float)(s->time+((s->flags & SAMPLE_ADD_OFFSET)?0.5:0)));
  else
	print("%u", s->time);      //Note: %lu --> %u for unsigned int

	eye =hasleft?0:1;
	for(; eye<(hasright?2:1); eye++)
	{
		float raw_pupil[2];
		float raw_cr[2];
		unsigned int pupil_area;
		unsigned int cr_area;
		UINT32 pupil_dimension[2];
		

		edf_get_uncorrected_raw_pupil(combfile,s, eye,raw_pupil);
		edf_get_uncorrected_raw_cr(combfile,s, eye, raw_cr);
		pupil_area = edf_get_uncorrected_pupil_area(combfile,s, eye);
		cr_area = edf_get_uncorrected_cr_area(combfile,s, eye);
		edf_get_pupil_dimension(combfile,s, eye,pupil_dimension);
		
		
		print_value3(raw_pupil[0],3); // rpx
		print_value3(raw_pupil[1],3); // rpy
		print_value((float)pupil_area,1); // pa
		//print_value(pupil_dimension[0],1);// pwidth
		//print_value(pupil_dimension[1],1);// pheight
		print_value3(raw_cr[0],3); //rcrx
		print_value3(raw_cr[1],3); // rcry
		print_value((float)cr_area,1); // rcra

		if(NaN == raw_pupil[0] || NaN == raw_cr[0])
			print_value3(NaN,3); //pupil-crx
		else
			print_value3(raw_pupil[0]-raw_cr[0],3); //pupil-crx

		if(NaN == raw_pupil[1] || NaN == raw_cr[1])
			print_value3(NaN,3); //pupil-cry
		else
			print_value3(raw_pupil[1]-raw_cr[1],3); // pupil-cry
			
		

		if((s->flags & SAMPLE_HEADPOS) && (s->htype==0xC4 || s->htype==0xC8))
		{
			cr_area = edf_get_uncorrected_cr2_area(combfile,s, eye);
			edf_get_uncorrected_raw_cr2(combfile,s, eye, raw_cr);
			print_value3(raw_cr[0],3); //rcrx
			print_value3(raw_cr[1],3); // rcry
			print_value((float)cr_area,1); // pa
		}
		
	  }
  
  
  return 0;
}
