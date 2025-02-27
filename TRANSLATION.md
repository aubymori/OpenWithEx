# Translation
Rather than translating strings\* and dialogs directly, they should be obtained from their respective Windows versions in that locale.

<small>\*`IDS_ERR_` strings should be translated directly.</small>

Translations should be submitted as RC files, not compiled MUIs.

## How to extract `shell32.dll` resources from a Windows ISO
You will need [7-Zip](https://7-zip.org/) for all of these methods.
You should use [Resource Hacker](https://www.angusj.com/resourcehacker/) to view the resources from the extracted file.

## Windows Vista/7
1. Open the ISO in 7-Zip.
2. Open `sources\install.wim`.
3. If there are numbered directories, click one.
4. Drag the file at `Windows\System32\<YOUR LOCALE CODE>\shell32.dll.mui` to some other folder.

## Windows NT 4.0/2000/XP
1. Open the ISO in 7-Zip.
2. Go to the folder matching the architecture of the iso (most likely `I386` or `AMD64`).
3. Go inside `SHELL32.DL_`, and drag `shell32.dll` to some other folder.

## Windows 95/98
1. Extract the ISO to a folder using 7-Zip.
2. Go to either the `win95` or `win98` folder, depending on which version you are using.
3. Open one of the `WIN9X_XX.CAB` files in 7-Zip.
4. Drag `shell32.dll` to some other folder.