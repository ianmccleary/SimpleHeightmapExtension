# Simple Heightmap Extension
A purposefully simple Heightmap that can be created and edited right in Godot.

![Alt text](/screenshots/overview.png "Simple Heightmap Extension Overview")

## Use
To create, add a **Simple Heightmap** node. When selected, the Simple Heightmap tools will appear on the right of the scene view.
* Raise/Lower: Click to raise terrain, and shift-click to lower terrain.
* Smooth: Click to smooth terrain.
* Flatten: Click to flatten terrain. The height it will flatten to is based on where you first click.
* Textures: Click on the image of the texture you wish to paint. The textures can be set by changing the properties of the Simple Heightmap in the Inspector.

* Radius: The radius of the brush.
* Strength: How much strength to apply to the selected tool.
* Ease: Controls how strength is tapered towards the edges of the brush. At 0, the brush will change all affected points exactly the same. At higher values, the brush will smooth changes towards the edges.
