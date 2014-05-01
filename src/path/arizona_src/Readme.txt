The software in the loop simulation of Gavilan Peak and Daisy Mountain, Anthem, AZ consists of two parts: a VISSIM model and two RSE applications.

The VISSIM model is in:
mmitss\Networks\Anthem\simulation\Daisy_Gavilan_Actuation
The drivermodel.dll is in the same folder of the VISSIM model, you need to require ASC3 license from PTV to run Econolite ASC3 virtual controller software.
There is a file named ConfigInfo.txt, you need to change the ip address to the device which runs the RSE applications. Port number can be remained the same.
If you want to model other intersections, you need to modify the reference point in the drivermodel.dll source code and recompile it. The reference point should be GPS
coordinates of the center of the new intersection. They can be found in DriverModel.cpp Lines 109-115.


Currently, the RSE applications are run in Savari RSE. The source code is compiled in the SDK provided by Savari. 

The two applications need a intersection map which is located in (you may need to change the path in the source code to read the map, so does the path of the log files)

mmitss\Networks\Anthem\configuration\GavlinPeakandDaisyMtn\MAP


The source code is located in:
mmitss\source\Anthem\MRP_EquippedVehicleTrajectoryAwareness\MMITSS_rsu_BSM_Receiver_Gavilan
mmitss\source\Anthem\MRP_PerformanceObserver\MMITSS_rsu_PerformanceObserver_Gavilan

The MMITSS_rsu_BSM_Receiver_Gavilan is the MRP_EquippedVehicleTrajectoryAware component, you can refer to the design document for the details.
The MMITSS_rsu_PerformanceObserver_Gavilan just receives the trajecotry message from MMITSS_rsu_BSM_Receiver_Gavilan, unpack it, and save to a data structure.
You can develop your own applications on how to use the trajectory data.
