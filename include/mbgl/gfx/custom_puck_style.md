# Custom puck style

This document describes a JSon format for styling and animating the puck.

The puck style will be defined in a JSON format. The format provides the following concepts:

## Variants

It is possible to define a default style then define multiple variants of the style where a subset of styling parameters are specialized. This way the style can be specialized differently for different displays or for different times of day, i.e: dark mode and day mode. In the style JSON the variant needs to be declared by filling the `variants` array. An example of style JSON with variants declaration:

```
{
    "variants": [
        "day_ic",
        "night_ic",
        "sat_day_ic",
        "sat_night_ic",
        "day_cid",
        "night_cid",
        "sat_day_cid",
        "sat_night_cid"
    ]
}
```

## Icons

The style allows to switch between icons or interpolate between icons. Similar to variants, the icons need to be declared in the `icons` array. Every array entry is a (key, value) pair where the key is a name for the icon and the value is a file or url path for the icon. The following example is a style declaring two variants: day and night and declares six icons:

```
{
    "variants": [
        "day",
        "night"
    ],
    "icons": [
        {"default_day" : "path/puck_day.png"},
        {"default_night" : "path/puck_day.png"},
        {"blind_l" : "path/adas_blind_spot_warning_left.png"},
        {"blind_r" : "path/adas_blind_spot_warning_right.png"},
        {"lane_keep" : "path/adas_lane_keep.png"},
        {"lane_depart" : "path/adas_lane_departure.png"}
    ]
}
```

## States

States correspond to different puck states. Each state is an object defining what icons are used and how they are styled and animated. Examples of states:

- DefaultState: The default state of in a navigation application
- ActiveAdasDefaultState: The default state when an ADAS system is engaged
- ActiveAdasBlindSpotLeftState: blind spot warning
- ActiveAdasBlindSpotRightState: blind spot warning L2

In each state it is possible to select one or two icons: `icon_1` and `icon_2`. It's not possible to select more than two icons to avoid a large combination of shader objects in the renderer. The following style defines the default state with one static icon that is different in day and night modes:

```
{
    "variants": [
        "day",
        "night"
    ],
    "icons": [
        {"default_day" : "path/puck_day.png"},
        {"default_night" : "path/puck_night.png"}
    ],
    "states": {
        "DefaultState": [
            {
                "icon_1": "default_day"

            },
            {
                "variant": "night",
                "icon_1": "default_night"
            }
        ]
    }
}
```

It is possible to add an animation by defining the variable `animation_duration`. `Animation_duration` is the length of the animation in seconds. The variable `animation_loop` repeats the animation when it is true. In the following example the puck icon is enlarged to a 1.5 scale in the Adas state:

```
{
    "variants": [
        "day",
        "night"
    ],
    "icons": [
        {"default_day" : "path/puck_day.png"},
        {"default_night" : "path/puck_night.png"}
    ],
    "states": {
        "DefaultState": [
            {
                "icon_1": "default_day"
            },
            {
                "variant": "night",
                "icon_1": "default_night"
            }
        ],
        "AdasState": [
            {
                "animation_duration" : 1.5,
                "icon_1": "default_day",
                "icon_1_scale": [0, 1, 1, 1.5]

            },
            {
                "variant": "night",
                "icon_1": "default_night"
            }
        ]
    }
}
```

`icon_1_scale` is either a number defining the scale or an array of pairs where the first value is the normalized animation time in the range [0, 1] where 0 is the start of the animation and 1 is the end of the animation (corresponding to the duration). The second value of the pair is the scale. Other values that can be changed are:

- `icon1_scale` and `icon2_scale` : default is 1. A value less than 1 mignifies the icon while a value greater than 1 magnifies the icon
- `icon1_opacity` and `icon2_opacity` : default is 1. 0 means fully transparent while 1 means fully opaque
- `icon1_mirror_x` and `icon2_mirror_x`: default is false. When true the icon is mirrored horizontally otherwise the icon is unchanged
- `icon1_mirror_y` and `icon2_mirror_y`: default is false. When true the icon is mirrored vertically otherwise the icon is unchanged
- `icon1_color` and `icon2_color`: default is [1, 1, 1]. RGB color multiplied by the icon color

Icons must be authored in 2D. All transformations such as scale or mirror are applied in 2D with the origin set at the center of the icon then the icon is projected onto the map. All icons have the same size on screen when the scale is 1 regardless of the pixel resolution of the icon.
Icons are not compressed in video memory so it is important to minimize the number of icons and use the smallest possible resolution such as 128x128.

An example that uses all parameters:

```
{
    "variants": [
        "day",
        "night"
    ],
    "icons": [
        {"default_day" : "path/puck_day.png"},
        {"default_night" : "path/puck_night.png"},
        {"blind_l" : "path/adas_blind_spot_warning_left.png"},
        {"blind_r" : "path/adas_blind_spot_warning_right.png"},
        {"lane_keep" : "path/adas_lane_keep.png"},
        {"lane_depart" : "path/adas_lane_departure.png"}
    ],
    "states": {
        "DefaultState": [
            {
                "icon_1": "default_day"
            },
            {
                "variant": "night",
                "icon_1": "default_night"
            }
        ],
        "ActiveAdasDefaultState": [
            {
                "animation_duration" : 1.5,
                "icon_1": "default_day",
                "icon_1_scale": [0, 1, 1, 1.5]
            },
            {
                "variant": "night",
                "icon_1": "default_night"
            }
        ],
        "ActiveAdasBlindSpotL1State": [
            {
                "animation_duration" : 2,
                "animation_loop" : true,
                "icon_1": "default_day",
                "icon_1_scale": 1.5,
                "icon_2": "blind_l",
                "icon_2_scale": [0, 2, 0.5, 4, 1, 2],
                "icon_2_opacity": [0, 0, 1, 1],
                "icon_2_color": [1, 1, 0]

            },
            {
                "variant": "night",
                "icon_1": "default_night",
                "icon_2_color": [0.7, 0.7, 0]
            }
        ],
        "ActiveAdasBlindSpotL2State": [
            {
                "animation_duration" : 2,
                "animation_loop" : true,
                "icon_1": "default_day",
                "icon_1_scale": 1.5,
                "icon_2": "blind_l",
                "icon_2_scale": [0, 2, 0.5, 4, 1, 2],
                "icon_2_opacity": [0, 0, 1, 1],
                "icon_2_color": [1, 0, 0]

            },
            {
                "variant": "night",
                "icon_1": "default_night",
                "icon_2_color": [0.7, 0, 0]
            }
        ],
        "ActiveAdasBlindSpotR1State": [
            {
                "animation_duration" : 2,
                "animation_loop" : true,
                "icon_1": "default_day",
                "icon_1_scale": 1.5,
                "icon_2": "blind_l",
                "icon2_mirror_x": true,
                "icon_2_scale": [0, 2, 0.5, 4, 1, 2],
                "icon_2_opacity": [0, 0, 1, 1],
                "icon_2_color": [1, 1, 0]

            },
            {
                "variant": "night",
                "icon_1": "default_night",
                "icon_2_color": [0.7, 0.7, 0]
            }
        ]
    }
}
```

## MapLibre API

The puck style json string needs to be passed to MapLibre when the Location component is initialized using the method `LocationComponentActivationOptions.customPuckStyle`. The LocationComponent initialization code would look like:

```
locationComponent.apply {
    val builder = LocationComponentActivationOptions.Builder(
        context,
        style
    ).locationComponentOptions(locationComponentDefaultOptions)
     .useDefaultLocationEngine(false)
     .customPuckAnimationEnabled(true)
     .customPuckAnimationIntervalMS(30)
     .customPuckLagMS(400)
     .customPuckStyle(json_string)
     .build()
}
```

The following MapLibreMap API allows the Kotlin client to select the variant and the state:

```
class MapLibreMap {
  void setCustomPuckVariant(String variant);
  void setCustomPuckSate(String state);
}
```

example

```
MapLibreMap.setCustomPuckVariant("night");
MapLibreMap.setCustomPuckSate("AdasState");
```
