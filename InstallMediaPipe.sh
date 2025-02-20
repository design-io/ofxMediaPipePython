#!/bin/bash

# Media pipe GPU support
# https://developers.google.com/mediapipe/framework/getting_started/gpu_support

function getCondaEnvPath {
	envs_info=$(conda info --envs)
	env_name=$1
	#echo "Environment name: ${env_name}."
	# Find the path of the specified environment
	env_path=$(echo "$envs_info" | grep -E "^$env_name\s+" | awk '{print $NF}')
	echo $env_path
}


conda_is_already_installed=0
# Check if the `conda` command is available in the PATH
if command -v conda &> /dev/null; then
	os_arch=$(uname -m)
    echo "Conda is installed arch: ${os_arch}."
    conda_is_already_installed=1
else
    echo "Conda is not installed."
fi

# conda install -n base libarchive -c main --force-reinstall --solver classic

if [ $conda_is_already_installed = 0 ]; then
#	https://docs.anaconda.com/free/miniconda/
	# macOS
	# https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-x86_64.sh
	# https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-arm64.sh

	# linux
	# https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh

	# macOS
	mkdir -p ~/miniconda3
	
	echo "Going to download miniconda."

	if [[ "$OSTYPE" =~ ^darwin ]]; then
		echo "Detected macOS"
		conda_download_link_type="Miniconda3-latest-MacOSX-arm64.sh"
		# Check architecture
		os_arch=$(uname -m)
		if [[ $os_arch == "arm64" ]]; then
			echo "ARM64 architecture"
		elif [[ $os_arch == "x86_64" ]]; then
			echo "x86_64 architecture"
			conda_download_link_type="Miniconda3-latest-MacOSX-x86_64.sh"
		else
			echo "Unknown architecture: $os_arch"
		fi
		curl https://repo.anaconda.com/miniconda/${conda_download_link_type} -o ~/miniconda3/miniconda.sh
		bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
	elif [[ "$OSTYPE" =~ ^linux ]]; then
		echo "Detected linux."
		conda_download_link_type="Miniconda3-latest-Linux-x86_64.sh"
		wget https://repo.anaconda.com/miniconda/${conda_download_link_type} -O ~/miniconda3/miniconda.sh
		bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
	fi
	
	source ~/miniconda3/etc/profile.d/conda.sh

	rm -rf ~/miniconda3/miniconda.sh

	~/miniconda3/bin/conda init bash
	~/miniconda3/bin/conda init zsh

	# do not automatically start base environment
	conda config --set auto_activate_base false
	
	if [[ "$OSTYPE" =~ ^darwin ]]; then
		# reinit bash profile
		source ~/.bash_profile
	elif [[ "$OSTYPE" =~ ^linux ]]; then
		# reinit bash profile
		source ~/.bash_profile
	fi

fi

echo "Going to check if there is already a mediapipe environment."
conda_env_name="mediapipe"

env_path=$(getCondaEnvPath ${conda_env_name})
# Print the path of the environment
echo "Path of '$conda_env_name' environment: $env_path"

if [ -z "$env_path" ]; then
	echo "Could not find the ${conda_env_name} path."
	echo "Going to create a conda environment named ${conda_env_name}."
	conda init
	conda create --name $conda_env_name python=3.11.8 pip=23.3.1
	echo "Going to activate conda environment named ${conda_env_name}."
	conda activate $conda_env_name
	conda env list
	echo "---------------------------------";

	if [[ "$OSTYPE" =~ ^darwin ]]; then
		echo "Installing wget."
		conda install wget
	fi
	echo "Going to install opencv to ${conda_env_name}."
	#conda install opencv=4.9.0 -c conda-forge
	echo "Going to install mediapipe to ${conda_env_name}."
	pip install mediapipe==0.10.9
else
	echo "${conda_env_name} conda environment detected, not recreating."
fi

env_path=$(getCondaEnvPath ${conda_env_name})
if [ -z "$env_path" ]; then
	echo "ERROR: Unable to create ${conda_env_name} in conda. :("
	exit 1
fi

# Print the path of the environment
echo "Path of '$conda_env_name' environment: $env_path"

install_addon_lib_name="osx"
lib_ext="dylib"
if [[ "$OSTYPE" =~ ^linux ]]; then
	install_addon_lib_name="linux64"
	lib_ext="so"
fi

python_lib_name="libpython3.11.${lib_ext}"
python_dylib="${env_path}/lib/${python_lib_name}"
python_addon_install_dir="libs/python/lib/${install_addon_lib_name}"
python_addon_install_path="${python_addon_install_dir}/${python_lib_name}"

if [ -e ${python_addon_install_path} ]; then
	echo "Already have ${python_addon_install_path}, not copying."
else 
	if [ -e ${python_dylib} ]; then
		echo "Found the python lib at ${python_dylib}."
		echo "Copying to ${python_addon_install_path}."
		cp $python_dylib $python_addon_install_path
	else
		echo "ERROR: Cound NOT find the python lib at ${python_dylib}."
		exit 1
	fi
fi

# if there is another python lib that matches the name, copy it over
python_lib_name="libpython3.11.${lib_ext}.1.0"
python_dylib="${env_path}/lib/${python_lib_name}"
python_addon_install_dir="libs/python/lib/${install_addon_lib_name}"
python_addon_install_path="${python_addon_install_dir}/${python_lib_name}"

if [ -e ${python_addon_install_path} ]; then
	echo "Already have ${python_addon_install_path}, not copying."
else 
	if [ ! -d $python_addon_install_dir ]; then 
		mkdir -p "$python_addon_install_dir"
	fi

	if [ -e ${python_dylib} ]; then
		echo "Found the python lib at ${python_dylib}."
		echo "Copying to ${python_addon_install_path}."
		cp $python_dylib $python_addon_install_path
	else
		echo "ERROR: Cound NOT find the python lib at ${python_dylib}."
		#exit 1
	fi
fi

python_include_dir_name="python3.11"
python_include_dir="${env_path}/include/${python_include_dir_name}"
python_addon_include_dir="libs/python/include"
python_addon_include_path="${python_addon_include_dir}/${python_include_dir_name}"


if [ -d $python_addon_include_path ]; then
	echo "Already have ${python_addon_include_path}, not copying."
else
	if [ ! -d $python_addon_include_dir ]; then
		mkdir -p "$python_addon_include_dir"
	fi

	if [ -d $python_include_dir ]; then
		echo "Found the python headers at $python_include_dir."
		echo "Copying to ${python_addon_include_path}."
		cp -r $python_include_dir $python_addon_include_path
	else
		echo "ERROR: Could NOT find the python headers at $python_include_dir."
		exit 1
	fi
fi

# echo "-----------------------------------------------"
# echo "Successfully installed ${conda_env_name} to ${env_path}."
# echo "Successfully copied ${python_dylib} to ${python_addon_install_path}."
# echo "Successfully copied ${python_include_dir} to ${python_addon_include_path}."
# echo "-----------------------------------------------"
exit 0
