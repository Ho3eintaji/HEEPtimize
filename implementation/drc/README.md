```
WAIT!!!!
```

If you came here to do `drc`, indeed this is what this folder was meant for...

but you know, it is the day before the dealine, and we have prepared all the drc flow in the `../dummyfill`
folder... so just go there and read the `Makefile`

I leave here what it was a `work-in-progress` readme:

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Ok, you want to do the `DRC` here.

First thing, did you add the dummy fill metal and poly odi?

if not, go to the `dummyfill` folder:

```
cd ../dummyfill
```

and then `make`.

Now you are ready:

```
/softs/mentor/calibre/2022.2/bin/calibredrv ../dummyfill/heepocrates_filled.gds.gz -l /dkits/tsmc/65nm/IP_65nm/CALIBRE_LAYER/65nm_9m6x1z1u.layerprops
```

Click on `Verification`, `Run nmDRC`.

In the `Rules` add the `rules_to_use`, which is a symbolic link to the one from the `PDK`.

