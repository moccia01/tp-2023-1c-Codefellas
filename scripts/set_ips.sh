#!/bin/bash

# Verificar si se proporcionaron 4 argumentos de IP
if [ "$#" -ne 4 ]; then
  echo "Se requieren 4 IPs como argumentos."
  exit 1
fi

# Array con los nombres de directorio
directorios=("kernel" "cpu" "fileSystem" "memoria")

# Array con los nombres de los archivos de configuración
configs=("BASE" "DEADLOCK" "ERROR" "MEMORIA" "FS")


# Recorrer los directorios
for i in "${!directorios[@]}"; do
  dir="${directorios[$i]}"
  ip="${!i+1}"

  # Verificar si el directorio existe
  if [ -d "$dir" ]; then
    config_dir="$dir/configs"

    # Verificar si el directorio de configuración existe
    if [ -d "$config_dir" ]; then
      # Recorrer los archivos de configuración en el directorio
      for config in "${configs[@]}"; do
        config_file="$config_dir/$config.config"

        # Verificar si el archivo de configuración existe
        if [ -f "$config_file" ]; then
          # Actualizar las líneas en el archivo de configuración
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
        else
          echo "El archivo $config_file no existe."
        fi
      done
      echo "Modulo $dir actualizado."
    else
      echo "El directorio de configuración $config_dir no existe en $dir."
    fi
  else
    echo "El directorio $dir no existe."
  fi
done
