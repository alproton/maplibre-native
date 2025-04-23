package org.maplibre.android.testapp.activity.routes

import android.graphics.Color
import android.util.Log
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.RouteID
import org.maplibre.geojson.Point
import timber.log.Timber

data class RouteCircle(
    var resolution : Double,
    var xlate : Double,
    var radius : Double,
    var numTrafficZones : Int,
    var points : ArrayList<Point>
) {
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
            val dx = p2.longitude() - p1.longitude()
            val dy = p2.latitude() - p1.latitude()
            val length = Math.sqrt(dx * dx + dy * dy)
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

        fun disposeRoute(mapView : MapView) {
            if (routeMap.isEmpty()) return

            val firstRouteID : RouteID = routeMap.keys.first()
            if(firstRouteID.isValid()) {
                mapView.disposeRoute(firstRouteID)
            }
            routeMap.remove(firstRouteID)
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

        fun setPointProgress(mapView: MapView, progress: Double) {
            val routeID = routeMap.keys.first()
            val routeCircle = routeMap[routeID]
            if(routeCircle == null) return

            val point = routeCircle.getPoint(progress)
            Timber.tag("RouteProgress").i("getPoint: $point")
            val calculatedPercent = mapView.setRouteProgressPoint(routeID, point, true, false)
            Timber.tag("RouteProgress").i("inputPercent: $progress , calculatedPercent: $calculatedPercent")

//            mapView.setCustomPuckState(point.latitude(), point.longitude(), 0.0, 1.0f, false)

        }

        fun setPercentProgress(mapView: MapView, percent: Double) {
            val routeID = routeMap.keys.first()
            val routeCircle = routeMap[routeID]
            if(routeCircle == null) return

            mapView.setRouteProgress(routeID, percent)
        }

        fun addRoute(mapView : MapView) {
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
            routeMap[routeID] = routeCircle

            addTrafficSegments(routeID, mapView)
        }
    }
}
