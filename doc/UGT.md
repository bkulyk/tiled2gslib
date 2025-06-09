Note: this is the doc from UGT, keeping it here for reference.

# UGT Documentation

The UGT app is provided with the GSL library to create metatile and scrolltable data for the library.
Using the tool you can import an image or tiled file and have it create tiles, palette(s), metatiles and a scrolltable. The tool can also convert between image files and tiled files so should you have your desired map in an image file but wish to add meta elements via the tiled app you can use this tool.

## Tiles Image File UGT Palette

### Tiled File Metatiles Scrolltable

1. Import > open image file.
2. Select the image file you wish to import and press open.
3. If the image file is ok (dimensions or multiple of 16, etc) UGT will display the image file along with information regarding tiles, palettes, metatiles etc.
4. (optional) Customize VRAM layout of tiles. In the Tiles tab tile layout text area enter a range(s) you wish tiles to be allocated then press Update button. You can chain ranges (0,488)(506,507) etc.
The left will display image representing tiles as they will appear in vram. The pink color represents empty space in vram.
Default option is to place graphics starting from position 0.
5. (optional) Customize Palette. Click the Palette tab and press the Custom Palette radio button. In the custom palette text area you will see a list of colour hex values in the same order as they will be currently output.
Re-arrange those colours to select your desired palette layout then press update
6. Export required files. You need to export tiles, palette, metatiles and scrolltable. Choose each of these options in the menu and select .bin as the export format in save dialog.
Once this is done you have all required files for GSL.

### Tiled Documents to GSL Files

Converting tiled document to GSL files is overall a very similar process to converting an image to GSL files with one major exception. The tiled document MUST contain the following layers...

- GSLMetaLayer (used to denote 3 most significant bits in nametable entries)
- GSLCollisionLayer (work in progress, don’t use this layer)
- GSLPriorityLayer (used to denote priority bit for nametable entries)
- GSLTileLayer (graphics representing map to be scrolled)

In addition it must contain two tile sets with specific names. Both must be embedded tilesets (see small icons below tileset image display).

- GSLMeta (Use the tileset image included with this document)
- GSLTiles (tiles for your map)

Along with the UGT Tool is an empty Tiled document set up with these layers and an additional meta tile set to use with GSLMetaLayer and GSLPriorityLayer.
Alternatively if you import an image in to UGT and export it as a Tiled Document it will appear as per the original image with these required layers and the meta tile set.

## PRIORITY LAYER

The priority layer is self-explanatory. Any tile location that is not empty will be considered high priority. In the meta tile set I included a P for easy identification but it can be any non-empty tile.

## META LAYER

Meta layer sets the 3 most significant bits of each of the 4 nametable entries in a metatile. Each nametable entry can have a meta element of 0-7, 0 being default (nothing stored in meta layer).
In the GSL demo we had interactive and non-interactive tombstones. The meta layer were used to distinguish between tombstone metatiles by modifying the nametable entries. When parsed by UGT 3 tombstones metatiles will be created to encapsulate all variations. We can then differentiate for our interactivity. See UGT Metatiles below for more information.

## UGT Metatiles Tab

Interactivity requires that we can identify / distinguish between metatiles when looked up by GSL library. The Metatiles tab provides a list of all metatiles along with info regarding priority and most significant bits. Note you can only add more information to metatiles via tiled documents.

### Metatile Order

Metatiles are not stored in a linear 1 to 255. This is due to entries being stored in a post shifted form for speed on the library side. UGT Still orders them from smallest index to largest though you will see gaps. This does not affect the running of things but mentioned here as I imagine it is something that would be normally asked.

### Priority Bits

Priority bits are signified via the area directly below the metatile image. Normally this is empty representing no priority. When priority is set it this section to pink in the same formation as the above metatile.

## Meta Bits

The 3 most significant bits for each nametable entry are displayed as numbers under the metatile number.

## UGT Command Line

UGT supports command line input for batch processing. UGT can take either an image or tiled doc as well as destination and name prefix for files and produce scrolltable, metatile, tile and palette binary files.
Command line options are as follows
-tiledfile <url>: location of tiled document you wish to process -imagefile <url>: location of image file you wish to process -destination <url>: location to save 4 binary documents. -name <text>: prefix name for tiles.
The following command line....

```
java -jar ugt.jar -tiledfile “C:\game\resources\stage1.tmx” - destination “C:\game\assets” -name stage1
```

will produce files...

stage1_scrolltable.bin, stage1_metatiles.bin, stage1_tiles.bin, stage1_palette.bin.

----

## GSLib

### Data Formats

#### Metatiles

Metatiles are stored as raw Nametable entries in order left to right, top to bottom for a total of 4 entries (8 bytes). Metatile index 0 is used to contain meta information for the table and is not used.

Meta information for index 0 is as follows (in same order as file format)...

- (2 bytes) Length of metatile table in bytes
- (6 bytes) unused

#### Scrolltable Data

Scrolltable data is a representation of map using metatiles. Entries are stored as modified Metatile entries in order left to right, top to bottom. The modified metatile format is (metatile_id << 3) & 248) + ((metatile_id >> 5) & 7).

Scrolltable contains a header before metatile entries. Header is 13 bytes long and is structured...

- (2 bytes) GSL_ScrolltableSize - size in bytes
- (2 bytes) GSL_WidthInMetatiles - width in metatile entries
- (2 bytes) GSL_HeightInMetatiles - height in metatile entries
- (2 bytes) GSL_WidthInPixels
- (2 bytes) GSL_HeightInPixels
- (2 bytes) GSL_VerticalAddition - width in metatiles * 13
- (1 bytes) GSL_OptionByte (Lookups require generation of table, highest bit signals to generate table).
