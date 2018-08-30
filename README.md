# 4NXCI

![License](https://img.shields.io/badge/license-ISC-blue.svg)

4NXCI is a tool for converting XCI(NX Card Image) files to NSP  

4NXCI is based on hactool by SciresM [hactool](https://github.com/SciresM/hactool)  
Thanks: SciresM, Rajkosto, Switch Brew

## Usage
You need to place your keyset file with "keys.dat" filename in the same folder as 4NXCI  
Required keys are:  

Key Name | Description
-------- | -----------
header_key | NCA Header Key
key_area_key_application_xx | Application key area encryption keys

Created NSP files are located in the same folder as 4NXCI with 'titleid.nsp' filename  
4NXCI creates "4nxci_extracted_xci" folder as a temp folder, you can remove it after the conversion is done

```
*nix: ./4nxci <path_to_file.xci>  
Windows: .\4nxci.exe <path_to_file.xci>  
```

## Licensing

This software is licensed under the terms of the ISC License.  
You can find a copy of the license in the LICENSE file.
