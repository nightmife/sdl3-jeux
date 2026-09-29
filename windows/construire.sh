#!/usr/bin/env bash
# Construit la version WINDOWS du jeu (SANS triches) depuis Linux, et la range
# dans un dossier + un .zip prêts à envoyer.
# Utilisation (depuis ~/claudeCours/sdl3-jeux) :  ./windows/construire.sh
set -euo pipefail                  # s'arrêter à la moindre erreur
cd "$(dirname "$0")/.."            # se placer à la racine du projet

# 1. Configurer : compilateur croisé MinGW, sans triches, optimisé (Release)
cmake -S . -B build-windows \
      -DCMAKE_TOOLCHAIN_FILE=windows/mingw-toolchain.cmake \
      -DAVEC_TRICHES=OFF \
      -DCMAKE_BUILD_TYPE=Release

# 2. Compiler uniquement le jeu en ligne
cmake --build build-windows --target en_ligne -j

# 3. Rassembler tout ce qu'il faut pour jouer sous Windows
DEST=build-windows/ChasseAuxPieces
EXE=build-windows/15-en-ligne/en_ligne.exe
rm -rf "$DEST" && mkdir -p "$DEST"
cp "$EXE" "$DEST/ChasseAuxPieces.exe"
cp -r 15-en-ligne/assets "$DEST/"
cp windows/deps/SDL3-*/x86_64-w64-mingw32/bin/SDL3.dll             "$DEST/"
cp windows/deps/SDL3_image-*/x86_64-w64-mingw32/bin/SDL3_image.dll "$DEST/"
cp windows/deps/SDL3_ttf-*/x86_64-w64-mingw32/bin/SDL3_ttf.dll     "$DEST/"

# "Stripper" l'exe : retirer la table des symboles (les noms des fonctions,
# inutiles pour jouer). Plus petit, et plus rien qui trahisse le code source.
x86_64-w64-mingw32-strip "$DEST/ChasseAuxPieces.exe"

# DLL du compilateur MinGW dont l'exe a encore besoin (objdump liste les DLL
# qu'un .exe importe). On stocke d'abord la liste dans une variable : avec
# "pipefail", "objdump | grep -q" échouerait même quand grep trouve (grep -q
# s'arrête tôt et coupe objdump, ce qui compte comme une erreur).
IMPORTS=$(x86_64-w64-mingw32-objdump -p "$EXE")
for dll in libwinpthread-1.dll libgcc_s_seh-1.dll libstdc++-6.dll; do
    if grep -qi "DLL Name: $dll" <<< "$IMPORTS"; then
        cp "/usr/x86_64-w64-mingw32/bin/$dll" "$DEST/"
    fi
done

# 4. Le .zip à envoyer
rm -f build-windows/ChasseAuxPieces.zip
(cd build-windows && python3 -m zipfile -c ChasseAuxPieces.zip ChasseAuxPieces)

echo
echo "Version Windows prête :"
echo "  dossier : $DEST"
echo "  archive : build-windows/ChasseAuxPieces.zip"
ls -la "$DEST"
