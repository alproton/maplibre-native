package org.maplibre.android.testapp.activity.routes

import android.graphics.Color
import android.util.Log
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.RouteID
import org.maplibre.geojson.Point
import timber.log.Timber
import kotlin.math.*

fun haversineDist(p1: Point, p2: Point): Double {
    val EARTH_RADIUS_METERS = 6371000.0

    fun degreesToRadians(degrees: Double): Double {
        return degrees * PI / 180.0
    }

    val p1Latitude = p1.latitude()
    val p1Longitude = p1.longitude()
    val p2Latitude = p2.latitude()
    val p2Longitude = p2.longitude()

    val lat1Rad = degreesToRadians(p1Latitude)
    val lon1Rad = degreesToRadians(p1Longitude)
    val lat2Rad = degreesToRadians(p2Latitude)
    val lon2Rad = degreesToRadians(p2Longitude)

    val dLat = lat2Rad - lat1Rad
    val dLon = lon2Rad - lon1Rad

    val a = sin(dLat / 2.0).pow(2) +
            cos(lat1Rad) * cos(lat2Rad) * sin(dLon / 2.0).pow(2)
    val c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a))

    return EARTH_RADIUS_METERS * c
}


data class RouteCircle(
    var resolution : Double,
    var xlate : Double,
    var radius : Double,
    var numTrafficZones : Int,
    var points : ArrayList<Point>
) {
    var totalRouteLenth : Double = 0.0

    fun getTotalLength() : Double {
        if(totalRouteLenth > 0.0) return totalRouteLenth

        totalRouteLenth = 0.0
        for (i in 0 until points.size - 1) {
            val p1 = points[i]
            val p2 = points[i + 1]
            val length = haversineDist(p1, p2)
            totalRouteLenth += length
        }
        return totalRouteLenth
    }

    fun getPoint(percent : Double) : Point {
        Timber.tag("General").i("percent: $percent")
        if(points.isEmpty()) return Point.fromLngLat(0.0, 0.0)
        if(percent < 0.0) return points[0]
        if(percent > 1.0) return points[points.size - 1]

        var totalLength = 0.0
        var segmentLengths : ArrayList<Double> = ArrayList()
        for (i in 0 until points.size - 1) {
            val p1 = points[i]
            val p2 = points[i + 1]
            val length = haversineDist(p1, p2)
            totalLength += length
            segmentLengths.add(length)
        }

        val targetLength : Double = percent * totalLength
        var accumulatedLength : Double = 0.0

        for(i in 0 .. segmentLengths.size - 1) {
            val segmentLength = segmentLengths[i]
            if(accumulatedLength + segmentLength >= targetLength) {
                val p1 = points[i]
                val p2 = points[i + 1]
                val dx = p2.longitude() - p1.longitude()
                val dy = p2.latitude() - p1.latitude()
                val segmentPercent = (targetLength - accumulatedLength) / segmentLength
                return Point.fromLngLat(p1.longitude() + dx * segmentPercent,
                    p1.latitude() + dy * segmentPercent)
            }
            accumulatedLength += segmentLength
        }

        return points[points.size - 1]
    }
}

data class TrafficBlock(
    var block : MutableList<Point>,
    var priority : Int,
    var color : Int,
)

data class TrafficBlockFracational (
    var firstIndex : Int,
    var firstIndexFractional : Float,
    var lastIndex: Int,
    var lastIndexFractional : Float,
    var priority : Int,
    var color : Int,
)

class RouteUtils {
    companion object {
        var routeMap : MutableMap<RouteID, RouteCircle> = mutableMapOf()
        var progressModePoint : Boolean = false

        fun createRouteCircle(resolution: Double, radius: Double, numTrafficZones: Int): RouteCircle {
            val points = ArrayList<Point>()
            val xlate = routeMap.size * radius * 2.0
            for (i in 0 until resolution.toInt()) {
                val angle = 2 * Math.PI * i / (resolution-1)
                val x = xlate + radius * Math.cos(angle)
                val y = radius * Math.sin(angle)
                points.add(Point.fromLngLat(x, y))
            }

            return RouteCircle(resolution, xlate, radius, numTrafficZones, points)
        }

        fun getTotalRouteLength() : Double {
            if (routeMap.isEmpty()) return 0.0
            val routeID = routeMap.keys.first()

            val routeCircle = routeMap[routeID]
            if(routeCircle == null) return 0.0

            return routeCircle.getTotalLength()
        }

        fun disposeRoute(mapView : MapView) {
            if (routeMap.isEmpty()) return

            val firstRouteID : RouteID = routeMap.keys.first()
            if(firstRouteID.isValid()) {
                mapView.disposeRoute(firstRouteID)
            }
            routeMap.remove(firstRouteID)
            mapView.finalizeRoutes()
        }

        fun addTrafficSegmentsFractional(routeID : RouteID, mapView : MapView) {
            var trafficBlks : MutableList<TrafficBlockFracational> = mutableListOf()
            val numPts = routeMap[routeID]?.points?.size ?: 0
            val trafficBlk0 = TrafficBlockFracational(0, 0.0f, 0, 1.0f, 0, Color.RED)
            val trafficBlk1 = TrafficBlockFracational(numPts-2, 0.0f, numPts-2, 1.0f, 0, Color.GREEN)
            val trafficBlk2 = TrafficBlockFracational(1, 0.2f, 1, 0.8f, 0, Color.CYAN)
            val trafficBlk3 = TrafficBlockFracational(2, 0.5f, 3, 0.5f, 0, Color.YELLOW)
            val trafficBlk4 = TrafficBlockFracational(4, 0.0f, 5, 1.0f, 0, Color.DKGRAY)

            trafficBlks.add(trafficBlk0)
            trafficBlks.add(trafficBlk1)
            trafficBlks.add(trafficBlk2)
            trafficBlks.add(trafficBlk3)
            trafficBlks.add(trafficBlk4)

            mapView.clearRouteSegments(routeID)
            for(trafficBlk in trafficBlks) {
                val rsopts = org.maplibre.android.maps.RouteSegmentOptions()
                rsopts.color = trafficBlk.color
                rsopts.outerColor = Color.BLACK
                rsopts.priority = trafficBlk.priority
                rsopts.firstIndex = trafficBlk.firstIndex
                rsopts.firstIndexFraction = trafficBlk.firstIndexFractional
                rsopts.lastIndex = trafficBlk.lastIndex
                rsopts.lastIndexFraction = trafficBlk.lastIndexFractional
                mapView.createRouteSegmentFractional(routeID, rsopts)
            }
            mapView.finalizeRoutes()
        }

        fun addTrafficSegments(routeID : RouteID, mapView : MapView) {
            val segmentColors : IntArray = intArrayOf(
                Color.RED,
                Color.GREEN,
                Color.CYAN,
                Color.YELLOW,
                Color.WHITE,
                Color.MAGENTA
            )

            var trafficBlks : MutableList<TrafficBlock> = mutableListOf()
            var routeCircle = routeMap[routeID]
            val blockSize = Math.floor(routeCircle!!.resolution / routeCircle.numTrafficZones).toInt()
            val innerBlockSize = Math.ceil(blockSize / 2.0).toInt()

            var currTrafficBlk: TrafficBlock = TrafficBlock(mutableListOf(), 0, Color.RED)
            var colorIdx : Int = 0
            for (i in 0 until routeCircle.points.size) {

                if ((i % blockSize) == 0 && !currTrafficBlk.block.isEmpty()) {
                    colorIdx++
                    currTrafficBlk.color = segmentColors[colorIdx % segmentColors.size]
                    trafficBlks.add(currTrafficBlk)
                    currTrafficBlk = TrafficBlock(mutableListOf(), 0, currTrafficBlk.color)
                }

                if ((i % blockSize) < innerBlockSize) {
                    currTrafficBlk.block.add(routeCircle.points[i])
                }
            }
            if(!currTrafficBlk.block.isEmpty()) {
                trafficBlks.add(currTrafficBlk)
            }

            mapView.clearRouteSegments(routeID)
            for(trafficBlk in trafficBlks) {
                val rsopts = org.maplibre.android.maps.RouteSegmentOptions()
                rsopts.color = trafficBlk.color
                rsopts.outerColor = Color.BLACK
                rsopts.priority = trafficBlk.priority
                rsopts.geometry = org.maplibre.geojson.LineString.fromLngLats(trafficBlk.block)
                mapView.createRouteSegment(routeID, rsopts)
            }
            mapView.finalizeRoutes()
        }

        fun setPointProgress(mapView: MapView, progress: Double, progressPrecisionCoarse: Boolean = true) {
            val routeID = routeMap.keys.first()
            val routeCircle = routeMap[routeID]
            if(routeCircle == null) return

            val point = routeCircle.getPoint(progress)
            Timber.tag("RouteProgress").i("getPoint: $point")
            val calculatedPercent = mapView.setRouteProgressPoint(routeID, point, progressPrecisionCoarse, false)
            Timber.tag("RouteProgress").i("inputPercent: $progress , calculatedPercent: $calculatedPercent")
        }

        fun getPointProgress(progress: Double) : Point {
            val routeID = routeMap.keys.first()
            val routeCircle = routeMap[routeID]

            return routeCircle?.getPoint(progress)!!
        }

        fun addRoute(mapView : MapView, useFractionalSegments : Boolean = false) {
            val segmentColors : IntArray = intArrayOf(
                Color.BLUE,
                Color.DKGRAY
            )

            val routeCircle = createRouteCircle(30.0, 5.0, 5)
            val points = routeCircle.points
            val routeGeometry = org.maplibre.geojson.LineString.fromLngLats(points)
            val routeOptions = org.maplibre.android.maps.RouteOptions()
            routeOptions.innerWidth = 14.0
            routeOptions.outerWidth = 16.0
            routeOptions.innerColor = if (routeMap.size == 0) segmentColors[0] else segmentColors[1]
            routeOptions.outerColor = Color.RED

            val routeID = mapView.createRoute(routeGeometry, routeOptions)
            mapView.finalizeRoutes()
            //set the first routeID as the vanishing route
            if  (routeMap.isEmpty()) {
                mapView.setVanishingRoute(routeID)
            }
            routeMap[routeID] = routeCircle

            if(useFractionalSegments) {
                addTrafficSegmentsFractional(routeID, mapView)
            } else {
                addTrafficSegments(routeID, mapView)
            }
        }
    }
}
