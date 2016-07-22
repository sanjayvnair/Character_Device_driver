 #!/bin/bash 
if [ $# -eq 0 ]
  then
    echo "No arguments supplied: taking default num_devices value of 3"
    sudo insmod ./char_driver.ko
    sudo chmod 777 /dev/mycdrv0 /dev/mycdrv1 /dev/mycdrv2
  else	
	devices=$(($1 - 1))
	sudo insmod ./char_driver.ko NUM_DEVICES=$1 
	for i in `seq 0 $devices`;
		do	
			sudo chmod 777 /dev/mycdrv$i
		done
fi            
