#!/bin/bash

cd fileSystem/archivos/

# Elimino todo al carajo y lo vuelvo a crear
rm bitmap.dat
rm bloques.dat
rm -r fcb/
mkdir fcb
touch bitmap.dat 
touch bloques.dat

# Leo los numeros del archivo "superbloque.dat"
while IFS='=' read -r clave valor || [ -n "$clave" ]; do
  if [[ "$clave" == "BLOCK_SIZE" ]]; then
    block_size="${valor//[!0-9]/}"
  elif [[ "$clave" == "BLOCK_COUNT" ]]; then
    block_count="${valor//[!0-9]/}"
  fi
done < "superbloque.dat"

# Verifico si los numeros fueron encontrados
if [[ -z "$block_size" ]] || [[ -z "$block_count" ]]; then
  echo "No se encontraron los números BLOCK_SIZE y BLOCK_COUNT en el archivo 'superbloque.dat'."
  exit 1
fi

# Calculo tam_bloques
tam_bloques=$((block_size * block_count))
tam_bitmap=$((block_count / 8))

# Truncate con los tamaños correspondientes
truncate -s "${tam_bitmap}" bitmap.dat
truncate -s "${tam_bloques}" bloques.dat

echo "bitmap.dat reseteado con tamaño ${tam_bitmap} Bytes"
echo "bloques.dat reseteado con tamaño ${tam_bloques} Bytes"