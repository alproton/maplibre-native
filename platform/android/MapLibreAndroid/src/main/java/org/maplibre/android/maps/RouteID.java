package org.maplibre.android.maps;

import java.util.Objects;

final public class RouteID {
    private int id;
    public RouteID(int id) {
        this.id = id;
    }
    public int getId() {
        return id;
    }
    public boolean isValid() {
        return id != -1;
    }

    @Override
    public int hashCode() {
        return Objects.hashCode(id);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj == null || getClass() != obj.getClass()) return false;
        RouteID other = (RouteID) obj;
        return id == other.id;
    }

    @Override
    public String toString() {
        return "RouteID{" + "id=" + id + '}';
    }
}
