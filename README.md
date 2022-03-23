# Porymap With Curry's Tools

[English version](#english) **apology to my English writing**

## 1. 关于

[Porymap](https://github.com/huderlem/porymap) 是一款强大而好用的地图编辑工具，用来编辑反编译宝可梦项目的地图，其易用性、方便性，深受用户的喜爱。

然而现在很多小组或个人仍在使用CFRU这样的hook形式，或者直接编辑rom来进行制作，使用的AdvanceMap稍显过时。得益于Porymap优秀的开发基础，我在上面填加了一些非常方便的功能。现在你可以方便的使用Porymap进行地图的编辑，将制作好的地图再导入AdvanceMap，或者用我这些小工具更方便的编辑你的反编译项目的地图。

感谢你花时间访问本项目，业余时间开发，代码水平不高，编程意识薄弱，可能出现bug，**请多多包涵、多多见谅，感谢**

## 2. 新功能

> **定义：在地图块编辑器中，左边的图叫Metatile，右边的图叫Tile**

> 功能1-5在`地图块编辑器-咖喱的小工具`

1. 压缩并导入图片到Tiles

    - **注意：该功能将以当前选择的Tile作为起始位置**

    将一个图片(必须被索引到16色)自动进行压缩，并填入Tiles，过程中需要的参数有
    - 预期宽度：图片填入Tile后所要使用的宽度
    - 跳过x块后插入：从当前选择位置算起，先跳过x块后插入。该功能用于不浪费每一块。
    - 是否允许插入空块：图片中是全部为索引颜色[0]的块(即透明块、空块)会被忽略。


2. 根据Tiles生成图片到Metatile

   - **注意：该功能将以当前选择的Metatile作为起始位置**

   - **注意：当前选择的Metatile的一二层Tiles、行为字节、等所有属性将被继承到写入的块**

    将一张图片(一般来说是第一步导入的图片)，对比Tiles中存在的块与当前选择的调色板，自动拼到Metatile。 然后得到一个辅助Json文件，该文件保存了Metatile到原图的拼装序列，主界面的"咖喱的小工具-导入Json到地图"功能使用这个文件将Metatile拼到地图上。过程中使用的参数有
    - 预期宽度：图片填入Metatile后所要使用的宽度
    - 跳过x块后插入：从当前选择位置算起，先跳过x块后插入。该功能用于不浪费每一块。
    - 是否允许插入空块：图片中是全部为索引颜色[0]的块(即透明块、空块)会被忽略。


3. 扩充Tiles

   扩充Primary、Secondary Tiles的数量。参数：
    - 数量：要扩充到的数量，必须大于当前值、小于最大值、被16整除


4. 导出AdvanceMap格式地图块

    导出AM中可以导入的Primary、Secondary地图块，包括bvd、dib、pal。参数：
    - 文件名：将以改文件名为起始，创建bvd、dib、pal


5. 导出当前调色板

   导出纯净的pal调色板。不带有莫名其妙的文件头
    - 文件名


6. 导入新版PS调色板

   **位于调色板编辑器**

    新版PS导出的pal调色板会有不正确的文件头，一般手动操作需要删除24字节，使用本工具可以直接导入不用。
     - 文件名


7. 导出AdvanceMap格式地图

    **位于主界面 咖喱的小工具**

    导出AM中可以导入的.map地图文件，包括bvd、dib、pal。由于AM设计缺陷，边缘地图快需要设置为2x2。参数：
    - 文件名


以上这些功能足以让你快速完成一张地图，enjoy！

## 3. 如何使用
本项目只增加了新的代码，没有动编译相关的东西，理论上你可以按照Porymap的使用说明 [INSTALL.md](INSTALL.md) 使用，或直接下载编译好的Release。

为了在Clion上使用，编写了Cmakelists，直接导入Clion后可以编译，但最终会报错，不过这不会影响代码提示和编写。

再次感谢你的访问和优秀的 [Porymap](https://github.com/huderlem/porymap) 项目

~~英文水平捉鸡 请见谅~~

-------

## 1. <span id="english">About</span>

[Porymap](https://github.com/huderlem/porymap) is a powerful and friendly map editor for Pokémon generation 3 decompilation projects. Hackers like it so much for its accessibility and convenience.

However, many hack groups still use the way of functions hook, such as CFRU, to edit a ROM, even modify ROM directly. Generally, we use AdvanceMap to modify maps, which is a little outdated. Benefit from the excellent bases of Porymap, I coded some very useful tools for Porymap. Now, on one hand, you can modify maps in Porymap and export your map to AdvanceMap formats, on the other hand, you can edit your maps in decompilation projects with more functional tools. 

**Thank you so much for your accessing**, I coded this project in my spare time, maybe there are some bugs.

Please excuse my poor coding and English writing.

Apology to not translate labels in program now, due to not all of my group members could read English. I will learn to add mutil-languages labels in this project in spare time.


## 2. What's new

> **The left image in Tileset Editor is named Metatile, the right one is Tile**

> tools 1-5 found in `Tileset Tditor-Curry Tools(咖喱的小工具)`

1. Compress and import image to Tiles (压缩并导入图片到Tiles)

    - **this function will start with current selected Tile**

    select and compress an indexed image, must be 16 colors, and import to Tiles. You need to set:
    - width (预期宽度)：the width of compressed image in Tiles
    - skip x Tiles (跳过x块后插入)：after current selected Tile, skip x Tiles first and import. This function is use for not waste any row. Parameters:
    - allow empty (是否允许插入空块)：the blank blocks in image will be ignored


2. Generate Metatiles by image and Tiles (根据Tiles生成图片到Metatile)

    - **this function will start with current selected Metatile**

    - **any imported blocks will use current selected Metatile's attribute, include bottom tiles, layer type, behavior...**

    auto generate Metatiles with selected image(generally the same as tool 1), palette, and tiles. After finishing, there is a Json file will be created, it is record the corresponding relationship with image and Metatiles. Using the Json file in `main window-Curry tools(咖喱的小工具)-Import Json to map(导入Json到地图)`. Parameters:
    - width (预期宽度)：the width of compressed image in Metatiles
    - skip x Tiles (跳过x块后插入)：after current selected Tile, skip x Tiles first and import. This function is use for not waste any row
    - allow empty (是否允许插入空块)：the blank blocks in image will be ignored


3. Expand tiles (扩充***Tiles)

   expand Primary or Secondary Tiles。Parameter:
    - nums (扩充到大小)：the num should greater than current, less than max, divided by 16


4. Export Tilesets in AdvanceMap format(导出AM格式的***地图块)

    export Primary or Secondary tilesets in AdvancedMap format, include bvd, dib, pal files. Parameter:
    - filename：start with filename, create bvd, dib, pal files


5. Export current palette(导出当前调色板)

    export pure pal format palette tile. Para:
    - filename


6. Import new Photoshop version pal file(导入新版PS调色板)

   - **found in `palette editor-tools`**

    new version of Photoshop will create palette with a header, this tool can import correctly. Para:
    - filename


7. export AdvanceMap map(导出AM格式地图)

    - **found in `main window-Curry tools(咖喱的小工具)-export AdvanceMap map(导出AM1.92地图)`**

    export .map file in AdvanceMap format. Due to tha AdvanceMap feature, you need to set border to 2x2. Para:
    - filename


## 3. How to use

For Windows users, download release.

Theoretically, you can refer to [INSTALL.md](INSTALL.md) by [Porymap](https://github.com/huderlem/porymap) , because I just add some code.

I add Cmakelists File to code in Clion, however, it can not finish compiling, but you can still use Clion code hints and code auxiliary tools

Thanks again to you and [Porymap](https://github.com/huderlem/porymap)
