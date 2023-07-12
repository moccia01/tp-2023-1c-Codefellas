#!/bin/bash

# Verifico si se proporcionaron 4 argumentos de IP
if [ "$#" -ne 4 ]; then
  echo "Se requieren 4 IPs como argumentos. Se reciben en orden: kernel, cpu, fs, memoria"
  exit 1
fi

# Argumentos recibidos por parametro
ip_kernel="$1"
ip_cpu="$2"
ip_fs="$3"
ip_memoria="$4"


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
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$ip_kernel"/; 2,4s/IP_MEMORIA=.*/IP_MEMORIA="$ip_memoria"/; 2,4s/IP_FILESYSTEM=.*/IP_FILESYSTEM="$ip_fs"/; 2,4s/IP_CPU=.*/IP_CPU="$ip_cpu"/" "$config_file"
        ;;
      "cpu")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$ip_cpu"/; 2s/IP_MEMORIA=.*/IP_MEMORIA="$ip_memoria"/" "$config_file"
        ;;
      "fileSystem")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$ip_fs"/; 2s/IP_MEMORIA=.*/IP_MEMORIA="$ip_memoria"/" "$config_file"
        ;;
      "memoria")
        sed -i "1s/IP_ESCUCHA=.*/IP_ESCUCHA="$ip_memoria"/" "$config_file"
        ;;
      *)
        echo "Directorio no reconocido: $dir"
        ;;
    esac

    echo "IP actualizada en $config_file"
  done
  echo "Modulo $dir actualizado."
done
