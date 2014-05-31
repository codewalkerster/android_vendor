package com.invensense.android.hardware.pedapi;

public class PedApi { 
  static {
      /*
       * Load the library.  If it's already loaded, this does nothing.
       */
      System.loadLibrary("mpl_ped_jni");
   }
  
  /* these are the pedometer interface functions (only useful if sys-ped is installed */
  public final static native int startPed();
  public final static native int stopPed();
  public final static native int getSteps();
  public final static native double getWalkTime();
  public final static native int clearPedData();
  
}
