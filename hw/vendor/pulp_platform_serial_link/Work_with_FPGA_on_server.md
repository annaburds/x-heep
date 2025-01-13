
## Compile on the Server and Emulate on FPGA

This is a guide for compiling hardware and software for x-heep on the ESL server and emulating it on the FPGA through the EPFL programmer (which must be programmed from your local machine). This can be done by mounting the server on your local folder.

### Prerequisites

You will need to be able to program the flash of the EPFL programmer. To install all the needed dependencies and libraries, follow the guide on [ProgramFlash tutorial](https://x-heep.readthedocs.io/en/latest/How_to/ProgramFlash.html).

To program the FPGA, you will need to first generate the bitstream following the steps on [ProgramFpga tutorial](https://x-heep.readthedocs.io/en/latest/How_to/RunOnFPGA.html).
Then, follow these steps:
1. Install Vivado Lab Edition (version 2022.2) on the local machine. It only requires about 8GB of disk space
2. (On the server) Generate bitstream
3. Download bitstream from the server
4. To make Vivado recognize the board, you first have to:
    - Download Board files for PYNQ-Z2 and copy the unzipped folder `pynq-z2` to `<Vivado Install Path>/data/boards/board_files`
    - Install the cable driver by executing the script `<Vivado Install Path>/data/xicom/cable_drivers/lin64/install_script/install_drivers/install_drivers`
5. Launch Vivado Lab Edition and program the FPGA via the GUI:
    - Open Hardware Manager
    - Open target -> Auto connect
    - Program Device


### Mount/Unmount the Server

To mount the server on your local folder, call the following script with the argument `-m`. To unmount it, call it with the argument `-u`.

```bash
#!/bin/bash
MOUNT_POINT="/your/local/folder/path/" # replace with X-Heep absolute path on your local machine
REMOTE_SERVER="server_name"   # replace with the server you want to use (e.g., eslsrv11)
REMOTE_DIR="/your/server/folder/path/" # replace with X-Heep absolute path on the ESL server
if [ "$1" == "-m" ]; then
  if mountpoint -q $MOUNT_POINT; then
    echo "Already mounted."
  else
    rsync -avz -e ssh $MOUNT_POINT $REMOTE_SERVER:$REMOTE_DIR --exclude .git --exclude build --exclude x-heep
    sshfs $REMOTE_SERVER:$REMOTE_DIR $MOUNT_POINT
    if [ $? -eq 0 ]; then
      echo "Mounted $REMOTE_SERVER:$REMOTE_DIR to $MOUNT_POINT"
    else
      echo "Failed to mount $REMOTE_SERVER:$REMOTE_DIR to $MOUNT_POINT"
    fi
  fi
elif [ "$1" == "-u" ]; then
  if mountpoint -q $MOUNT_POINT; then
    echo "Trying to unmount $MOUNT_POINT..."
    fusermount -u $MOUNT_POINT
    if [ $? -ne 0 ]; then
      echo "Unmount failed. Attempting to force unmount."
      lsof +D $MOUNT_POINT      # Lists processes using the mount point
      sudo fuser -k $MOUNT_POINT # Kills those processes
      sudo umount -l $MOUNT_POINT  # Force unmount
      if [ $? -eq 0 ]; then
        echo "Force unmounted $MOUNT_POINT"
      else
        echo "Force unmount failed. Manual intervention needed."
      fi
    else
      echo "Unmounted $MOUNT_POINT"
    fi
    rsync -avz -e ssh $REMOTE_SERVER:$REMOTE_DIR $MOUNT_POINT --exclude .git --exclude verible --exclude x-heep
  else
    echo "Not mounted."
  fi
else
  echo "Usage: $0 -m (mount) | -u (unmount)"
  exit 1
fi
```

### Compile and Generate Board Files on the Server and Program the FPGA 

**After mounting** the server folder on your local machine, you can generate board files, compile software on the server, and program the flash from your PC by running this script with the argument `-p` followed by the name of the project (application's folder name) you want to test.

```bash
#!/bin/bash
MOUNT_POINT="/your/local/folder/path/" # replace with X-Heep absolute path on your local machine
REMOTE_SERVER="server_name"   # replace with the server you want to use (e.g., eslsrv11)
REMOTE_DIR="/your/server/folder/path/" # replace with X-Heep absolute path on the ESL server
cmd="cd $REMOTE_DIR && conda activate core-v-mini-mcu && make app TARGET=pynq-z2 PROJECT=$project LINKER=flash_load " # modify this sequence of commands with what you need to perform on the server

if [ "$1" == "-h" ] || [ "$1" == "--help" ] || [ "$1" == "" ]; then
   echo "Usage: ./poluzzi -p <project> "
   echo "  -p: The name of the project to compile"
   exit 0
fi
while getopts ":p:t:f:c:" opt; do
  case $opt in
    p) project="$OPTARG";;
    t) target="$OPTARG";;
    f) flash="$OPTARG";;
    c) clean="$OPTARG";;
  esac
done
# Check if the mount point is already mounted
if mountpoint -q $MOUNT_POINT; then
  echo "Mount point is already mounted"
else
  echo "Mount point is not mounted. Please run mount script with -m option."
  exit 1
fi
cd $MOUNT_POINT
ssh $REMOTE_SERVER $cmd
make -C $MOUNT_POINT flash-prog
picocom -b 9600 -r -l --imap lfcrlf /dev/ttyUSB2 
```

### Workflow and Considerations

To work with these scripts effectively and avoid losing progress or overwriting files, follow this workflow:

1. First, when you start working on X-Heep, **mount** the server on your local X-Heep folder with the provided script. This pushes the status of your local folder to the server folder, then mounts the server folder on your local machine. Before doing this, ensure there are no changes on the server that are not reflected locally, as they will be overwritten by mounting.
2. Once mounted, your local folder essentially acts as a view of the server's folder. You can modify files on your local machine, and they will be reflected on the server. To compile and run code, use the provided script.
3. When you finish working on X-Heep, **unmount** the server with the provided script. This step is crucial because the script pulls the folder's status from the server to the local folder, which would otherwise revert to its state before mounting.

When following this workflow, always be mindful of where your latest changes are. Key points to consider:
 - When the server is mounted, you are making changes on the server and **not** on your local machine.
 - When you mount the server, the script overwrites the state of the remote folder with your local state.
 - When you unmount the server, the script overwrites the state of the local folder with the state of the remote folder.
 - You can manually push and pull the folder state to/from the server with the following command. Be cautious when doing this. You can add more `--exclude` arguments to ignore additional parts of X-Heep (also in the script) to speed up the operation.
    - To push code: `rsync -avz -e ssh /your/local/folder/path/ server_name:/your/server/folder/path/ --exclude .git --exclude build --exclude x-heep`
    - To pull code: `rsync -avz -e ssh server_name:/your/server/folder/path/ /your/local/folder/path/  --exclude .git --exclude x-heep` 
