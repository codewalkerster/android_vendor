package com.invensense.android.hardware.sysapi;

public class SysApi { 
  static {
      /*
       * Load the library.  If it's already loaded, this does nothing.
       */
      System.loadLibrary("mpl_sys_jni");
   }
  
  /** used to set the power state of  the various chips managed by the MPL*/
  public final static native int setSensors(long s);
  /** used to get the current power status of the various chips managed by 
   *  the MPL. The input aray should have length 1 
   * @param s array in which to store the return value
   * @return
   */
  public final static native int getSensors(long[] s);
  /** set the MPL bias update function mask */
  public final static native int setBiasUpdateFunc(long f);
  /** reset the compass calibration process */
  public final static native int resetCal();
  /** retrieve the sensor biases from the MPL. On return the biases are stored
   *  in the input array : accel (x,y,z) gyro (x,y,z), compass (x,y,z) */
  public final static native int getBiases(float[] b);   
  /** set the sensor biases that the MPL should use.  Overrides any internal
    * MPL bias calculation. The biases should be in the input array in this
    * order: accel (x,y,z) gyro (x,y,z), compass (x,y,z) */
  public final static native int setBiases(float[] b);
  /** run the mpl self test */
  public final static native int selfTest();
  /** set the local mag field */
  public final static native int setLocalMagField(float x, float y, float z);
  
  /* these are the pedometer interface functions (only useful if sys-ped is installed */
  public final static native int startPed();
  public final static native int stopPed();
  public final static native int getSteps();
  public final static native double getWalkTime();
  public final static native int clearPedData();
  
  
  
}