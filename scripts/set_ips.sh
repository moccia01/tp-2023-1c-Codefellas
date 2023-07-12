#!/bin/bash

# Verifico si se proporcionaron 4 argumentos de IP
if [ "$#" -ne 4 ]; then
  echo "Se requieren 4 IPs como argumentos. Se reciben en orden: kernel, cpu, fs, memoria"
  exit 1
fi

# Array con los nombres de directorio
directorios=("kernel" "cpu" "fileSystem" "memoria")

# Array con los nombres de los archivos de configuración
configs=("BASE" "DEADLOCK" "ERROR" "MEMORIA" "FS")

# Recorro los directorios
for dir in "${directorios[@]}"; do
  config_dir="$dir/configs"
  
  # Recorro los archivos de configuración en el directorio
  for config in "${configs[@]}"; do
    config_file="$config_dir/$config.config"

    # Actualizo las líneas en el archivo de configuración
    case $dir in
      "kernel")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$1"/; 2,4s/IP_MEMORIA=.*/IP_MEMORIA="$4"/; 2,4s/IP_FILESYSTEM=.*/IP_FILESYSTEM="$3"/; 2,4s/IP_CPU=.*/IP_CPU="$2"/" "$config_file"
        ;;
      "cpu")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$2"/; 2s/IP_MEMORIA=.*/IP_MEMORIA="$4"/" "$config_file"
        ;;
      "fileSystem")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$3"/; 2s/IP_MEMORIA=.*/IP_MEMORIA="$4"/" "$config_file"
        ;;
      "memoria")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$4"/" "$config_file"
        ;;
      *)
        echo "Directorio no reconocido: $dir"
        ;;
    esac

    echo "IP actualizada en $config_file"
  done
  echo "Modulo $dir actualizado."
done
