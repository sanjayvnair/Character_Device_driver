1.Place all the files in the same directory
2.makefile has been included in the directory.(Provided by TAs)
3.userapp for testing is also the same as provided by you.



Running programs:

1.Run the make file
2.A script has been attached with this file that runs all the commands as follows:

	insmod command
	chmod command for all the driver files generated

Please refer the run.sh file included in the directory for your reference.

Running command:
	sh  run.sh <NO_DEVICES>  ......... if no arguements are passed then 3 devices will be created




RUNNING WITHOUT SCRIPT (If Required)

make all
sudo insmod ./char_driver.ko
sudo chmod 777 /dev/mycdrv0 /dev/mycdrv1.... no of devices

./userapp ..... to test the driver



sudo rmmod char_driver ...... to remove driver



