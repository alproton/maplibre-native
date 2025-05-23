package org.maplibre.android.style.expressions;

import android.annotation.SuppressLint;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import org.maplibre.geojson.GeoJson;
import org.maplibre.geojson.Polygon;
import org.maplibre.geojson.gson.GeometryGeoJson;
import org.maplibre.android.style.layers.PropertyFactory;
import org.maplibre.android.style.layers.PropertyValue;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import static org.maplibre.android.utils.ColorUtils.colorToRgbaArray;

/**
 * The value for any layout property, paint property, or filter may be specified as an expression.
 * An expression defines a formula for computing the value of the property using the operators described below.
 * The set of expression operators provided by MapLibre GL includes:
 * <ul>
 * <li>Element</li>
 * <li>Mathematical operators for performing arithmetic and other operations on numeric values</li>
 * <li>Logical operators for manipulating boolean values and making conditional decisions</li>
 * <li>String operators for manipulating strings</li>
 * <li>Data operators, providing access to the properties of source features</li>
 * <li>Camera operators, providing access to the parameters defining the current map view</li>
 * </ul>
 * <p>
 * Expressions are represented as JSON arrays.
 * The first element of an expression array is a string naming the expression operator,
 * e.g. "*"or "case". Subsequent elements (if any) are the arguments to the expression.
 * Each argument is either a literal value (a string, number, boolean, or null), or another expression array.
 * </p>
 * <p>
 * Data expression: a data expression is any expression that access feature data -- that is,
 * any expression that uses one of the data operators:get,has,id,geometry-type, or properties.
 * Data expressions allow a feature's properties to determine its appearance.
 * They can be used to differentiate features within the same layer and to create data visualizations.
 * </p>
 * <p>
 * Camera expression: a camera expression is any expression that uses the zoom operator.
 * Such expressions allow the the appearance of a layer to change with the map's zoom level.
 * Camera expressions can be used to create the appearance of depth and to control data density.
 * </p>
 * <p>
 * Composition: a single expression may use a mix of data operators, camera operators, and other operators.
 * Such composite expressions allows a layer's appearance to be determined by
 * a combination of the zoom level and individual feature properties.
 * </p>
 * <p>
 * Example expression:
 * </p>
 * <pre>
 * {@code
 * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
 * fillLayer.setProperties(
 *   fillColor(
 *     interpolate( linear(), zoom(),
 *       stop(12, step(get("stroke-width"),
 *         color(Color.BLACK),
 *         stop(1f, color(Color.RED)),
 *         stop(2f, color(Color.WHITE)),
 *         stop(3f, color(Color.BLUE))
 *       )),
 *       stop(15, step(get("stroke-width"),
 *         color(Color.BLACK),
 *         stop(1f, color(Color.YELLOW)),
 *         stop(2f, color(Color.LTGRAY)),
 *         stop(3f, color(Color.CYAN))
 *       )),
 *       stop(18, step(get("stroke-width"),
 *         color(Color.BLACK),
 *         stop(1f, color(Color.WHITE)),
 *         stop(2f, color(Color.GRAY)),
 *         stop(3f, color(Color.GREEN))
 *       ))
 *     )
 *   )
 * );
 * }
 * </pre>
 */
public class Expression {

  @Nullable
  private final String operator;
  @Nullable
  private final Expression[] arguments;

  /**
   * Creates an empty expression for expression literals
   */
  Expression() {
    operator = null;
    arguments = null;
  }

  /**
   * Creates an expression from its operator and varargs expressions.
   *
   * @param operator  the expression operator
   * @param arguments expressions input
   */
  public Expression(@NonNull String operator, @Nullable Expression... arguments) {
    this.operator = operator;
    this.arguments = arguments;
  }

  /**
   * Create a literal number expression.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(literal(10.0f))
   * );
   * }
   * </pre>
   *
   * @param number the number
   * @return the expression
   */
  public static Expression literal(@NonNull Number number) {
    return new ExpressionLiteral(number);
  }

  /**
   * Create a literal string expression.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(literal("Text"))
   * );
   * }
   * </pre>
   *
   * @param string the string
   * @return the expression
   */
  public static Expression literal(@NonNull String string) {
    return new ExpressionLiteral(string);
  }

  /**
   * Create a literal boolean expression.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillAntialias(literal(true))
   * );
   * }
   * </pre>
   *
   * @param bool the boolean
   * @return the expression
   */
  public static Expression literal(boolean bool) {
    return new ExpressionLiteral(bool);
  }

  /**
   * Create a literal object expression.
   *
   * @param object the object
   * @return the expression
   */
  public static Expression literal(@NonNull Object object) {
    if (object.getClass().isArray()) {
      return literal(toObjectArray(object));
    } else if (object instanceof Expression) {
      throw new RuntimeException("Can't convert an expression to a literal");
    }
    return new ExpressionLiteral(object);
  }

  /**
   * Create a literal array expression
   *
   * @param array the array
   * @return the expression
   */
  public static Expression literal(@NonNull Object[] array) {
    return new Expression("literal", new ExpressionLiteralArray(array));
  }

  /**
   * Expression literal utility method to convert a color int to an color expression
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(color(Color.GREEN))
   * );
   * }
   * </pre>
   *
   * @param color the int color
   * @return the color expression
   */
  public static Expression color(@ColorInt int color) {
    float[] rgba = colorToRgbaArray(color);
    return rgba(rgba[0], rgba[1], rgba[2], rgba[3]);
  }

  /**
   * Creates a color value from red, green, and blue components, which must range between 0 and 255,
   * and an alpha component of 1.
   * <p>
   * If any component is out of range, the expression is an error.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *         rgb(
   *             literal(255.0f),
   *             literal(255.0f),
   *             literal(255.0f)
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param red   red color expression
   * @param green green color expression
   * @param blue  blue color expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#rgb">Style specification</a>
   */
  public static Expression rgb(@NonNull Expression red, @NonNull Expression green, @NonNull Expression blue) {
    return new Expression("rgb", red, green, blue);
  }

  /**
   * Creates a color value from red, green, and blue components, which must range between 0 and 255,
   * and an alpha component of 1.
   * <p>
   * If any component is out of range, the expression is an error.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *         rgb(255.0f, 255.0f, 255.0f)
   *     )
   * );
   * }
   * </pre>
   *
   * @param red   red color value
   * @param green green color value
   * @param blue  blue color value
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#rgb">Style specification</a>
   */
  public static Expression rgb(@NonNull Number red, @NonNull Number green, @NonNull Number blue) {
    return rgb(literal(red), literal(green), literal(blue));
  }

  /**
   * Creates a color value from red, green, blue components, which must range between 0 and 255,
   * and an alpha component which must range between 0 and 1.
   * <p>
   * If any component is out of range, the expression is an error.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *         rgba(
   *             literal(255.0f),
   *             literal(255.0f),
   *             literal(255.0f),
   *             literal(1.0f)
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param red   red color value
   * @param green green color value
   * @param blue  blue color value
   * @param alpha alpha color value
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#rgba">Style specification</a>
   */
  public static Expression rgba(@NonNull Expression red, @NonNull Expression green,
                                @NonNull Expression blue, @NonNull Expression alpha) {
    return new Expression("rgba", red, green, blue, alpha);
  }

  /**
   * Creates a color value from red, green, blue components, which must range between 0 and 255,
   * and an alpha component which must range between 0 and 1.
   * <p>
   * If any component is out of range, the expression is an error.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *         rgba(255.0f, 255.0f, 255.0f, 1.0f)
   *     )
   * );
   * }
   * </pre>
   *
   * @param red   red color value
   * @param green green color value
   * @param blue  blue color value
   * @param alpha alpha color value
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#rgba">Style specification</a>
   */
  public static Expression rgba(@NonNull Number red, @NonNull Number green, @NonNull Number blue, @NonNull Number
    alpha) {
    return rgba(literal(red), literal(green), literal(blue), literal(alpha));
  }

  /**
   * Returns a four-element array containing the input color's red, green, blue, and alpha components, in that order.
   *
   * @param expression an expression to convert to a color
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#to-rgba">Style specification</a>
   */
  public static Expression toRgba(@NonNull Expression expression) {
    return new Expression("to-rgba", expression);
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression("==", compareOne, compareTwo);
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                              @NonNull Expression collator) {
    return new Expression("==", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), true)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second boolean
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, boolean compareTwo) {
    return eq(compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return eq(compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, @NonNull String compareTwo,
                              @NonNull Expression collator) {
    return eq(compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     eq(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#==">Style specification</a>
   */
  public static Expression eq(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return eq(compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the input values are not equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression("!=", compareOne, compareTwo);
  }

  /**
   * Returns true if the input values are not equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                               @NonNull Expression collator) {
    return new Expression("!=", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the input values are equal, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), true)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second boolean
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(Expression compareOne, boolean compareTwo) {
    return new Expression("!=", compareOne, literal(compareTwo));
  }

  /**
   * Returns `true` if the input values are not equal, `false` otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return new Expression("!=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the input values are not equal, false otherwise.
   * The inputs must be numbers, strings, or booleans, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(@NonNull Expression compareOne, @NonNull String compareTwo,
                               @NonNull Expression collator) {
    return new Expression("!=", compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns `true` if the input values are not equal, `false` otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!=">Style specification</a>
   */
  public static Expression neq(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return new Expression("!=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is strictly greater than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gt(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E">Style specification</a>
   */
  public static Expression gt(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression(">", compareOne, compareTwo);
  }

  /**
   * Returns true if the first input is strictly greater than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gt(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E">Style specification</a>
   */
  public static Expression gt(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                              @NonNull Expression collator) {
    return new Expression(">", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the first input is strictly greater than the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gt(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E">Style specification</a>
   */
  public static Expression gt(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return new Expression(">", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is strictly greater than the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gt(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E">Style specification</a>
   */
  public static Expression gt(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return new Expression(">", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is strictly greater than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gt(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E">Style specification</a>
   */
  public static Expression gt(@NonNull Expression compareOne, @NonNull String compareTwo,
                              @NonNull Expression collator) {
    return new Expression(">", compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns true if the first input is strictly less than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lt(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C">Style specification</a>
   */
  public static Expression lt(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression("<", compareOne, compareTwo);
  }

  /**
   * Returns true if the first input is strictly less than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lt(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C">Style specification</a>
   */
  public static Expression lt(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                              @NonNull Expression collator) {
    return new Expression("<", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the first input is strictly less than the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lt(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C">Style specification</a>
   */
  public static Expression lt(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return new Expression("<", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is strictly less than the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lt(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C">Style specification</a>
   */
  public static Expression lt(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return new Expression("<", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is strictly less than the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lt(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C">Style specification</a>
   */
  public static Expression lt(@NonNull Expression compareOne, @NonNull String compareTwo,
                              @NonNull Expression collator) {
    return new Expression("<", compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns true if the first input is greater than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gte(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E%3D">Style specification</a>
   */
  public static Expression gte(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression(">=", compareOne, compareTwo);
  }

  /**
   * Returns true if the first input is greater than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gte(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E%3D">Style specification</a>
   */
  public static Expression gte(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                               @NonNull Expression collator) {
    return new Expression(">=", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the first input is greater than or equal to the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gte(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E%3D">Style specification</a>
   */
  public static Expression gte(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return new Expression(">=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is greater than or equal to the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     neq(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E%3D">Style specification</a>
   */
  public static Expression gte(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return new Expression(">=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is greater than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     gte(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3E%3D">Style specification</a>
   */
  public static Expression gte(@NonNull Expression compareOne, @NonNull String compareTwo,
                               @NonNull Expression collator) {
    return new Expression(">=", compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns true if the first input is less than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lte(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C%3D">Style specification</a>
   */
  public static Expression lte(@NonNull Expression compareOne, @NonNull Expression compareTwo) {
    return new Expression("<=", compareOne, compareTwo);
  }

  /**
   * Returns true if the first input is less than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lte(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second expression
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C%3D">Style specification</a>
   */
  public static Expression lte(@NonNull Expression compareOne, @NonNull Expression compareTwo,
                               @NonNull Expression collator) {
    return new Expression("<=", compareOne, compareTwo, collator);
  }

  /**
   * Returns true if the first input is less than or equal to the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lte(get("keyToValue"), 2.0f)
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C%3D">Style specification</a>
   */
  public static Expression lte(@NonNull Expression compareOne, @NonNull Number compareTwo) {
    return new Expression("<=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is less than or equal to the second, false otherwise.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lte(get("keyToValue"), "value")
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C%3D">Style specification</a>
   */
  public static Expression lte(@NonNull Expression compareOne, @NonNull String compareTwo) {
    return new Expression("<=", compareOne, literal(compareTwo));
  }

  /**
   * Returns true if the first input is less than or equal to the second, false otherwise.
   * The inputs must be numbers or strings, and both of the same type.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     lte(get("keyToValue"), get("keyToOtherValue"), collator(true, false))
   * );
   * }
   * </pre>
   *
   * @param compareOne the first expression
   * @param compareTwo the second String
   * @param collator   the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%3C%3D">Style specification</a>
   */
  public static Expression lte(@NonNull Expression compareOne, @NonNull String compareTwo,
                               @NonNull Expression collator) {
    return new Expression("<=", compareOne, literal(compareTwo), collator);
  }

  /**
   * Returns `true` if all the inputs are `true`, `false` otherwise.
   * <p>
   * The inputs are evaluated in order, and evaluation is short-circuiting:
   * once an input expression evaluates to `false`,
   * the result is `false` and no further input expressions are evaluated.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     all(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#all">Style specification</a>
   */
  public static Expression all(@NonNull Expression... input) {
    return new Expression("all", input);
  }

  /**
   * Returns `true` if any of the inputs are `true`, `false` otherwise.
   * <p>
   * The inputs are evaluated in order, and evaluation is short-circuiting:
   * once an input expression evaluates to `true`,
   * the result is `true` and no further input expressions are evaluated.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     any(get("keyToValue"), get("keyToOtherValue"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#any">Style specification</a>
   */
  public static Expression any(@NonNull Expression... input) {
    return new Expression("any", input);
  }

  /**
   * Logical negation. Returns `true` if the input is `false`, and `false` if the input is `true`.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     not(get("keyToValue"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!">Style specification</a>
   */
  public static Expression not(@NonNull Expression input) {
    return new Expression("!", input);
  }

  /**
   * Logical negation. Returns `true` if the input is `false`, and `false` if the input is `true`.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     not(false)
   * );
   * }
   * </pre>
   *
   * @param input boolean input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#!">Style specification</a>
   */
  public static Expression not(boolean input) {
    return not(literal(input));
  }

  /**
   * Selects the first output whose corresponding test condition evaluates to true.
   * <p>
   * For each case a condition and an output should be provided.
   * The last parameter should provide the default output.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     iconSize(
   *         switchCase(
   *             get(KEY_TO_BOOLEAN), literal(3.0f),
   *             get(KEY_TO_OTHER_BOOLEAN), literal(5.0f),
   *             literal(1.0f) // default value
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#case">Style specification</a>
   */
  public static Expression switchCase(@NonNull @Size(min = 1) Expression... input) {
    return new Expression("case", input);
  }

  /**
   * Selects the output whose label value matches the input value, or the fallback value if no match is found.
   * The `input` can be any string or number expression.
   * Each label can either be a single literal value or an array of values.
   * If types of the input and keys don't match, or the input value doesn't exist,
   * the expresion will fail without falling back to the default value.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textColor(
   *         match(get("keyToValue"),
   *             literal(1), rgba(255, 0, 0, 1.0f),
   *             literal(2), rgba(0, 0, 255.0f, 1.0f),
   *             rgba(0.0f, 255.0f, 0.0f, 1.0f)
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#match">Style specification</a>
   */
  public static Expression match(@NonNull @Size(min = 2) Expression... input) {
    return new Expression("match", input);
  }

  /**
   * Selects the output whose label value matches the input value, or the fallback value if no match is found.
   * The `input` can be any string or number expression.
   * Each label can either be a single literal value or an array of values.
   * If types of the input and keys don't match, or the input value doesn't exist,
   * the expresion will fail without falling back to the default value.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *   textColor(
   *     match(get("keyToValue"), rgba(0.0f, 255.0f, 0.0f, 1.0f),
   *       stop(1f, rgba(255, 0, 0, 1.0f)),
   *       stop(2f, rgba(0, 0, 255.0f, 1.0f))
   *     )
   *   )
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#match">Style specification</a>
   */
  public static Expression match(@NonNull Expression input, @NonNull Expression defaultOutput, @NonNull Stop... stops) {
    return match(join(join(new Expression[] {input}, Stop.toExpressionArray(stops)), new Expression[] {defaultOutput}));
  }

  /**
   * Evaluates each expression in turn until the first non-null value is obtained, and returns that value.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textColor(
   *         coalesce(
   *             get("keyToNullValue"),
   *             get("keyToNonNullValue")
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#coalesce">Style specification</a>
   */
  public static Expression coalesce(@NonNull Expression... input) {
    return new Expression("coalesce", input);
  }

  /**
   * Gets the feature properties object.
   * <p>
   * Note that in some cases, it may be more efficient to use {@link #get(Expression)}} instead.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-value", properties()))
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#properties">Style specification</a>
   */
  public static Expression properties() {
    return new Expression("properties");
  }

  /**
   * Gets the feature's geometry type: Point, MultiPoint, LineString, MultiLineString, Polygon, MultiPolygon.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(concat(get("key-to-value"), literal(" "), geometryType()))
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#geometry-types">Style specification</a>
   */
  public static Expression geometryType() {
    return new Expression("geometry-type");
  }

  /**
   * Gets the feature's id, if it has one.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(id())
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#id">Style specification</a>
   */
  public static Expression id() {
    return new Expression("id");
  }

  /**
   * Gets the value of a cluster property accumulated so far. Can only be used in the clusterProperties
   * option of a clustered GeoJSON source.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   *  GeoJsonOptions options = new GeoJsonOptions()
   *                              .withCluster(true)
   *                              .withClusterProperty("max", max(accumulated(), get("max")).toArray(), get("mag").toArray());
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#accumulated">Style specification</a>
   */
  public static Expression accumulated() {
    return new Expression("accumulated");
  }

  /**
   * Gets the kernel density estimation of a pixel in a heatmap layer,
   * which is a relative measure of how many data points are crowded around a particular pixel.
   * Can only be used in the `heatmap-color` property.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * HeatmapLayer layer = new HeatmapLayer("layer-id", "source-id");
   * layer.setProperties(
   *     heatmapColor(interpolate(linear(), heatmapDensity(),
   *         literal(0), rgba(33, 102, 172, 0),
   *         literal(0.2), rgb(103, 169, 207),
   *         literal(0.4), rgb(209, 229, 240),
   *         literal(0.6), rgb(253, 219, 199),
   *         literal(0.8), rgb(239, 138, 98),
   *         literal(1), rgb(178, 24, 43))
   *     )
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#heatmap-density">Style specification</a>
   */
  public static Expression heatmapDensity() {
    return new Expression("heatmap-density");
  }

  /**
   * Gets the progress along a gradient line. Can only be used in the line-gradient property.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * LineLayer layer = new LineLayer("layer-id", "source-id");
   * layer.setProperties(
   *     lineGradient(interpolate(
   *         linear(), lineProgress(),
   *         stop(0f, rgb(0, 0, 255)),
   *         stop(0.5f, rgb(0, 255, 0)),
   *         stop(1f, rgb(255, 0, 0)))
   *     )
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#line-progress">Style specification</a>
   */
  public static Expression lineProgress() {
    return new Expression("line-progress");
  }

  /**
   * Retrieves an item from an array.
   *
   * @param number     the index expression
   * @param expression the array expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#at">Style specification</a>
   */
  public static Expression at(@NonNull Expression number, @NonNull Expression expression) {
    return new Expression("at", number, expression);
  }

  /**
   * Retrieves an item from an array.
   *
   * @param number     the index expression
   * @param expression the array expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#at">Style specification</a>
   */
  public static Expression at(@NonNull Number number, @NonNull Expression expression) {
    return at(literal(number), expression);
  }

  /**
   * Retrieves whether an item exists in an array or a substring exists in a string.
   *
   * @param needle   the item expression
   * @param haystack the array or string expression
   * @return true if exists.
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#in">Style specification</a>
   */
  public static Expression in(@NonNull Expression needle, @NonNull Expression haystack) {
    return new Expression("in", needle, haystack);
  }

  /**
   * Returns the first position at which a `needle` can be found in a `haystack`.
   *
   * @param needle   the item expression
   * @param haystack the array or string expression
   * @return position in the array or string or -1 if not found.
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#index-of">Style specification</a>
   */
  public static Expression indexOf(@NonNull Expression keyword, @NonNull Expression input) {
    return new Expression("index-of", keyword, input);
  }

  /**
   * Returns the first position at which a `needle` can be found in a `haystack`.
   *
   * @param needle   the item expression
   * @param haystack the array or string expression
   * @param fromIndex the index to start searching from
   * @return position in the array or string or -1 if not found.
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#index-of">Style specification</a>
   */
  public static Expression indexOf(@NonNull Expression keyword, @NonNull Expression input, @NonNull Expression fromIndex) {
    return new Expression("index-of", keyword, input, fromIndex);
  }

  /**
   * Returns items from an array or a substring from a string from a specified start index.
   * The return value is inclusive of the start index.
   *
   * @param input the array or string expression
   * @param fromIndex the index to start slice from
   * @return array or string
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#slice">Style specification</a>
   */
  public static Expression slice(@NonNull Expression input, @NonNull Expression fromIndex) {
    return new Expression("slice", input, fromIndex);
  }

  /**
   * Returns items from an array or a substring from a string between a start index and an end index if set.
   * The return value is inclusive of the start index, but not of the end index.
   *
   * @param input the array or string expression
   * @param fromIndex the index to start slice from
   * @param toIndex the index to end slice at
   * @return array or string
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#slice">Style specification</a>
   */
  public static Expression slice(@NonNull Expression input, @NonNull Expression fromIndex, @NonNull Expression toIndex) {
    return new Expression("slice", input, fromIndex, toIndex);
  }

  /**
   * Retrieves whether an item exists in an array or a substring exists in a string.
   *
   * @param needle   the item expression
   * @param haystack the array or string expression
   * @return true if exists.
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#in">Style specification</a>
   */
  public static Expression in(@NonNull Number needle, @NonNull Expression haystack) {
    return new Expression("in", literal(needle), haystack);
  }

  /**
   * Retrieves whether an item exists in an array or a substring exists in a string.
   *
   * @param needle   the item expression
   * @param haystack the array or string expression
   * @return true if exists.
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#in">Style specification</a>
   */
  public static Expression in(@NonNull String needle, @NonNull Expression haystack) {
    return new Expression("in", literal(needle), haystack);
  }

  /**
   * Retrieves the shortest distance between two geometries.
   * The returned value can be consumed as an input into another expression for changing a paint or layout property
   * or filtering features by distance.
   * <p>
   * Currently supports `Point`, `MultiPoint`, `LineString`, `MultiLineString` geometry types.
   *
   * @param geoJson the target feature geoJson.
   *                Currently supports `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`
   *                geometry types
   * @return the distance in the unit "meters".
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#distance">Style specification</a>
   */
  public static Expression distance(@NonNull GeoJson geoJson) {
    Map<String, Expression> map = new HashMap<>();
    map.put("json", literal(geoJson.toJson()));
    return new Expression("distance", new ExpressionMap(map));
  }

  public static Expression within(@NonNull Polygon polygon) {
    Map<String, Expression> map = new HashMap<>();

    map.put("type", literal(polygon.type()));
    map.put("json", literal(polygon.toJson()));

    return new Expression("within", new ExpressionMap(map));
  }

  /**
   * Retrieves a property value from the current feature's properties,
   * or from another object if a second argument is provided.
   * Returns null if the requested property is missing.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-feature"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#get">Style specification</a>
   */
  public static Expression get(@NonNull Expression input) {
    return new Expression("get", input);
  }

  /**
   * Retrieves a property value from the current feature's properties,
   * or from another object if a second argument is provided.
   * Returns null if the requested property is missing.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-feature"))
   * );
   * }
   * </pre>
   *
   * @param input string input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#get">Style specification</a>
   */
  public static Expression get(@NonNull String input) {
    return get(literal(input));
  }

  /**
   * Retrieves a property value from another object.
   * Returns null if the requested property is missing.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-property", get("key-to-object")))
   * );
   * }
   * </pre>
   *
   * @param key    a property value key
   * @param object an expression object
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#get">Style specification</a>
   */
  public static Expression get(@NonNull Expression key, @NonNull Expression object) {
    return new Expression("get", key, object);
  }

  /**
   * Retrieves a property value from another object.
   * Returns null if the requested property is missing.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-property", get("key-to-object")))
   * );
   * }
   * </pre>
   *
   * @param key    a property value key
   * @param object an expression object
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#get">Style specification</a>
   */
  public static Expression get(@NonNull String key, @NonNull Expression object) {
    return get(literal(key), object);
  }

  /**
   * Tests for the presence of an property value in the current feature's properties.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     has(get("keyToValue"))
   * );
   * }
   * </pre>
   *
   * @param key the expression property value key
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#has">Style specification</a>
   */
  public static Expression has(@NonNull Expression key) {
    return new Expression("has", key);
  }

  /**
   * Tests for the presence of an property value in the current feature's properties.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     has("keyToValue")
   * );
   * }
   * </pre>
   *
   * @param key the property value key
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#has">Style specification</a>
   */
  public static Expression has(@NonNull String key) {
    return has(literal(key));
  }

  /**
   * Tests for the presence of an property value from another object.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     has(get("keyToValue"), get("keyToObject"))
   * );
   * }
   * </pre>
   *
   * @param key    the expression property value key
   * @param object an expression object
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#has">Style specification</a>
   */
  public static Expression has(@NonNull Expression key, @NonNull Expression object) {
    return new Expression("has", key, object);
  }

  /**
   * Tests for the presence of an property value from another object.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setFilter(
   *     has("keyToValue", get("keyToObject"))
   * );
   * }
   * </pre>
   *
   * @param key    the property value key
   * @param object an expression object
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#has">Style specification</a>
   */
  public static Expression has(@NonNull String key, @NonNull Expression object) {
    return has(literal(key), object);
  }

  /**
   * Gets the length of an array or string.
   *
   * @param expression an expression object or expression string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#lenght">Style specification</a>
   */
  public static Expression length(@NonNull Expression expression) {
    return new Expression("length", expression);
  }

  /**
   * Gets the length of an array or string.
   *
   * @param input a string
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#lenght">Style specification</a>
   */
  public static Expression length(@NonNull String input) {
    return length(literal(input));
  }

  /**
   * Returns mathematical constant ln(2).
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(product(literal(10.0f), ln2()))
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#ln2">Style specification</a>
   */
  public static Expression ln2() {
    return new Expression("ln2");
  }

  /**
   * Returns the mathematical constant pi.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(product(literal(10.0f), pi()))
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#pi">Style specification</a>
   */
  public static Expression pi() {
    return new Expression("pi");
  }

  /**
   * Returns the mathematical constant e.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(product(literal(10.0f), e()))
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#e">Style specification</a>
   */
  public static Expression e() {
    return new Expression("e");
  }

  /**
   * Returns the sum of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sum(literal(10.0f), ln2(), pi()))
   * );
   * }
   * </pre>
   *
   * @param numbers the numbers to calculate the sum for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#+">Style specification</a>
   */
  public static Expression sum(@Size(min = 2) Expression... numbers) {
    return new Expression("+", numbers);
  }

  /**
   * Returns the sum of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sum(10.0f, 5.0f, 3.0f))
   * );
   * }
   * </pre>
   *
   * @param numbers the numbers to calculate the sum for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#+">Style specification</a>
   */
  @SuppressLint("Range")
  public static Expression sum(@Size(min = 2) Number... numbers) {
    Expression[] numberExpression = new Expression[numbers.length];
    for (int i = 0; i < numbers.length; i++) {
      numberExpression[i] = literal(numbers[i]);
    }
    return sum(numberExpression);
  }

  /**
   * Returns the product of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(product(literal(10.0f), ln2()))
   * );
   * }
   * </pre>
   *
   * @param numbers the numbers to calculate the product for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#*">Style specification</a>
   */
  public static Expression product(@Size(min = 2) Expression... numbers) {
    return new Expression("*", numbers);
  }

  /**
   * Returns the product of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(product(10.0f, 2.0f))
   * );
   * }
   * </pre>
   *
   * @param numbers the numbers to calculate the product for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#*">Style specification</a>
   */
  @SuppressLint("Range")
  public static Expression product(@Size(min = 2) Number... numbers) {
    Expression[] numberExpression = new Expression[numbers.length];
    for (int i = 0; i < numbers.length; i++) {
      numberExpression[i] = literal(numbers[i]);
    }
    return product(numberExpression);
  }

  /**
   * Returns the result of subtracting a number from 0.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(subtract(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number subtract from 0
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#-">Style specification</a>
   */
  public static Expression subtract(@NonNull Expression number) {
    return new Expression("-", number);
  }

  /**
   * Returns the result of subtracting a number from 0.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(subtract(10.0f))
   * );
   * }
   * </pre>
   *
   * @param number the number subtract from 0
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#-">Style specification</a>
   */
  public static Expression subtract(@NonNull Number number) {
    return subtract(literal(number));
  }

  /**
   * Returns the result of subtracting the second input from the first.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(subtract(literal(10.0f), pi()))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#-">Style specification</a>
   */
  public static Expression subtract(@NonNull Expression first, @NonNull Expression second) {
    return new Expression("-", first, second);
  }

  /**
   * Returns the result of subtracting the second input from the first.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(subtract(10.0f, 20.0f))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#-">Style specification</a>
   */
  public static Expression subtract(@NonNull Number first, @NonNull Number second) {
    return subtract(literal(first), literal(second));
  }

  /**
   * Returns the result of floating point division of the first input by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(division(literal(10.0f), pi()))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#/">Style specification</a>
   */
  public static Expression division(@NonNull Expression first, @NonNull Expression second) {
    return new Expression("/", first, second);
  }

  /**
   * Returns the result of floating point division of the first input by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(division(10.0f, 20.0f))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#/">Style specification</a>
   */
  public static Expression division(@NonNull Number first, @NonNull Number second) {
    return division(literal(first), literal(second));
  }

  /**
   * Returns the remainder after integer division of the first input by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(mod(literal(10.0f), pi()))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%25">Style specification</a>
   */
  public static Expression mod(@NonNull Expression first, @NonNull Expression second) {
    return new Expression("%", first, second);
  }

  /**
   * Returns the remainder after integer division of the first input by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(mod(10.0f, 10.0f))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%25">Style specification</a>
   */
  public static Expression mod(@NonNull Number first, @NonNull Number second) {
    return mod(literal(first), literal(second));
  }

  /**
   * Returns the result of raising the first input to the power specified by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(pow(pi(), literal(2.0f)))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%5E">Style specification</a>
   */
  public static Expression pow(@NonNull Expression first, @NonNull Expression second) {
    return new Expression("^", first, second);
  }

  /**
   * Returns the result of raising the first input to the power specified by the second.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(pow(5.0f, 2.0f))
   * );
   * }
   * </pre>
   *
   * @param first  the first number
   * @param second the second number
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#%5E">Style specification</a>
   */
  public static Expression pow(@NonNull Number first, @NonNull Number second) {
    return pow(literal(first), literal(second));
  }

  /**
   * Returns the square root of the input
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sqrt(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to take the square root from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#sqrt">Style specification</a>
   */
  public static Expression sqrt(@NonNull Expression number) {
    return new Expression("sqrt", number);
  }

  /**
   * Returns the square root of the input
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sqrt(25.0f))
   * );
   * }
   * </pre>
   *
   * @param number the number to take the square root from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#sqrt">Style specification</a>
   */
  public static Expression sqrt(@NonNull Number number) {
    return sqrt(literal(number));
  }

  /**
   * Returns the base-ten logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(log10(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to take base-ten logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#log10">Style specification</a>
   */
  public static Expression log10(@NonNull Expression number) {
    return new Expression("log10", number);
  }

  /**
   * Returns the base-ten logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(log10(10))
   * );
   * }
   * </pre>
   *
   * @param number the number to take base-ten logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#log10">Style specification</a>
   */
  public static Expression log10(@NonNull Number number) {
    return log10(literal(number));
  }

  /**
   * Returns the natural logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(ln(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to take natural logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#ln">Style specification</a>
   */
  public static Expression ln(Expression number) {
    return new Expression("ln", number);
  }

  /**
   * Returns the natural logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(ln(10))
   * );
   * }
   * </pre>
   *
   * @param number the number to take natural logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#ln">Style specification</a>
   */
  public static Expression ln(@NonNull Number number) {
    return ln(literal(number));
  }

  /**
   * Returns the base-two logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(log2(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to take base-two logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#log2">Style specification</a>
   */
  public static Expression log2(@NonNull Expression number) {
    return new Expression("log2", number);
  }

  /**
   * Returns the base-two logarithm of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(log2(2))
   * );
   * }
   * </pre>
   *
   * @param number the number to take base-two logarithm from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#log2">Style specification</a>
   */
  public static Expression log2(@NonNull Number number) {
    return log2(literal(number));
  }

  /**
   * Returns the sine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sin(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the sine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#sin">Style specification</a>
   */
  public static Expression sin(@NonNull Expression number) {
    return new Expression("sin", number);
  }

  /**
   * Returns the sine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(sin(90.0f))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the sine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#sin">Style specification</a>
   */
  public static Expression sin(@NonNull Number number) {
    return sin(literal(number));
  }

  /**
   * Returns the cosine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(cos(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the cosine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#cos">Style specification</a>
   */
  public static Expression cos(@NonNull Expression number) {
    return new Expression("cos", number);
  }

  /**
   * Returns the cosine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(cos(0))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the cosine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#cos">Style specification</a>
   */
  public static Expression cos(@NonNull Number number) {
    return new Expression("cos", literal(number));
  }

  /**
   * Returns the tangent of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(tan(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the tangent for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#tan">Style specification</a>
   */
  public static Expression tan(@NonNull Expression number) {
    return new Expression("tan", number);
  }

  /**
   * Returns the tangent of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(tan(45.0f))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the tangent for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#tan">Style specification</a>
   */
  public static Expression tan(@NonNull Number number) {
    return new Expression("tan", literal(number));
  }

  /**
   * Returns the arcsine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(asin(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arcsine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#asin">Style specification</a>
   */
  public static Expression asin(@NonNull Expression number) {
    return new Expression("asin", number);
  }

  /**
   * Returns the arcsine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(asin(90))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arcsine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#asin">Style specification</a>
   */
  public static Expression asin(@NonNull Number number) {
    return asin(literal(number));
  }

  /**
   * Returns the arccosine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(acos(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arccosine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#acos">Style specification</a>
   */
  public static Expression acos(@NonNull Expression number) {
    return new Expression("acos", number);
  }

  /**
   * Returns the arccosine of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(acos(0))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arccosine for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#acos">Style specification</a>
   */
  public static Expression acos(@NonNull Number number) {
    return acos(literal(number));
  }

  /**
   * Returns the arctangent of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(asin(pi()))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arctangent for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#atan">Style specification</a>
   */
  public static Expression atan(@NonNull Expression number) {
    return new Expression("atan", number);
  }

  /**
   * Returns the arctangent of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(atan(90))
   * );
   * }
   * </pre>
   *
   * @param number the number to calculate the arctangent for
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#atan">Style specification</a>
   */
  public static Expression atan(@NonNull Number number) {
    return atan(literal(number));
  }

  /**
   * Returns the minimum value of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(min(pi(), literal(3.14f), literal(3.15f)))
   * );
   * }
   * </pre>
   *
   * @param numbers varargs of numbers to get the minimum from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#min">Style specification</a>
   */
  public static Expression min(@Size(min = 1) Expression... numbers) {
    return new Expression("min", numbers);
  }

  /**
   * Returns the minimum value of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(min(3.141, 3.14f, 3.15f))
   * );
   * }
   * </pre>
   *
   * @param numbers varargs of numbers to get the minimum from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#min">Style specification</a>
   */
  @SuppressLint("Range")
  public static Expression min(@Size(min = 1) Number... numbers) {
    Expression[] numberExpression = new Expression[numbers.length];
    for (int i = 0; i < numbers.length; i++) {
      numberExpression[i] = literal(numbers[i]);
    }
    return min(numberExpression);
  }

  /**
   * Returns the maximum value of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(max(pi(), product(pi(), pi())))
   * );
   * }
   * </pre>
   *
   * @param numbers varargs of numbers to get the maximum from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#max">Style specification</a>
   */
  public static Expression max(@Size(min = 1) Expression... numbers) {
    return new Expression("max", numbers);
  }

  /**
   * Returns the maximum value of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(max(3.141, 3.14f, 3.15f))
   * );
   * }
   * </pre>
   *
   * @param numbers varargs of numbers to get the maximum from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#max">Style specification</a>
   */
  @SuppressLint("Range")
  public static Expression max(@Size(min = 1) Number... numbers) {
    Expression[] numberExpression = new Expression[numbers.length];
    for (int i = 0; i < numbers.length; i++) {
      numberExpression[i] = literal(numbers[i]);
    }
    return max(numberExpression);
  }

  /**
   * Rounds the input to the nearest integer.
   * Halfway values are rounded away from zero.
   * For example `[\"round\", -1.5]` evaluates to -2.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(round(pi()))
   * );
   * }
   * </pre>
   *
   * @param expression number expression to round
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#round">Style specification</a>
   */
  public static Expression round(Expression expression) {
    return new Expression("round", expression);
  }

  /**
   * Rounds the input to the nearest integer.
   * Halfway values are rounded away from zero.
   * For example `[\"round\", -1.5]` evaluates to -2.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(round(3.14159265359f))
   * );
   * }
   * </pre>
   *
   * @param number number to round
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#round">Style specification</a>
   */
  public static Expression round(@NonNull Number number) {
    return round(literal(number));
  }

  /**
   * Returns the absolute value of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(abs(subtract(pi())))
   * );
   * }
   * </pre>
   *
   * @param expression number expression to get absolute value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression abs(Expression expression) {
    return new Expression("abs", expression);
  }

  /**
   * Returns the absolute value of the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(abs(-3.14159265359f))
   * );
   * }
   * </pre>
   *
   * @param number number to get absolute value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression abs(@NonNull Number number) {
    return abs(literal(number));
  }

  /**
   * Returns the smallest integer that is greater than or equal to the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(ceil(pi()))
   * );
   * }
   * </pre>
   *
   * @param expression number expression to get value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression ceil(Expression expression) {
    return new Expression("ceil", expression);
  }

  /**
   * Returns the smallest integer that is greater than or equal to the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(ceil(3.14159265359))
   * );
   * }
   * </pre>
   *
   * @param number number to get value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression ceil(@NonNull Number number) {
    return ceil(literal(number));
  }

  /**
   * Returns the largest integer that is less than or equal to the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(floor(pi()))
   * );
   * }
   * </pre>
   *
   * @param expression number expression to get value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression floor(Expression expression) {
    return new Expression("floor", expression);
  }

  /**
   * Returns the largest integer that is less than or equal to the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(floor(pi()))
   * );
   * }
   * </pre>
   *
   * @param number number to get value from
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#abs">Style specification</a>
   */
  public static Expression floor(@NonNull Number number) {
    return floor(literal(number));
  }

  /**
   * Returns the IETF language tag of the locale being used by the provided collator.
   * This can be used to determine the default system locale,
   * or to determine if a requested locale was successfully loaded.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   * circleColor(switchCase(
   * eq(literal("it"), resolvedLocale(collator(true, true, Locale.ITALY))), literal(ColorUtils.colorToRgbaString
   * (Color.GREEN)),
   * literal(ColorUtils.colorToRgbaString(Color.RED))))
   * );
   * }
   * </pre>
   *
   * @param collator the collator expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#resolved-locale">Style specification</a>
   */
  public static Expression resolvedLocale(Expression collator) {
    return new Expression("resolved-locale", collator);
  }

  /**
   * Returns true if the input string is expected to render legibly.
   * Returns false if the input string contains sections that cannot be rendered without potential loss of meaning
   * (e.g. Indic scripts that require complex text shaping,
   * or right-to-left scripts if the the mapbox-gl-rtl-text plugin is not in use in MapLibre GL JS).
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * maplibreMap.getStyle().addLayer(new SymbolLayer("layer-id", "source-id")
   *   .withProperties(
   *     textField(
   *       switchCase(
   *         isSupportedScript(get("name_property")), get("name_property"),
   *         literal("not-compatible")
   *       )
   *     )
   *   ));
   * }
   * </pre>
   *
   * @param expression the expression to evaluate
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#is-supported-script">Style
   * specification</a>
   */
  public static Expression isSupportedScript(Expression expression) {
    return new Expression("is-supported-script", expression);
  }

  /**
   * Returns true if the input string is expected to render legibly.
   * Returns false if the input string contains sections that cannot be rendered without potential loss of meaning
   * (e.g. Indic scripts that require complex text shaping,
   * or right-to-left scripts if the the mapbox-gl-rtl-text plugin is not in use in MapLibre GL JS).
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * maplibreMap.getStyle().addLayer(new SymbolLayer("layer-id", "source-id")
   * .withProperties(
   *   textField(
   *     switchCase(
   *       isSupportedScript("ಗೌರವಾರ್ಥವಾಗಿ"), literal("ಗೌರವಾರ್ಥವಾಗಿ"),
   *       literal("not-compatible"))
   *     )
   *   )
   * );
   * }
   * </pre>
   *
   * @param string the string to evaluate
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#is-supported-script">Style
   * specification</a>
   */
  public static Expression isSupportedScript(@NonNull String string) {
    return new Expression("is-supported-script", literal(string));
  }

  /**
   * Returns the input string converted to uppercase.
   * <p>
   * Follows the Unicode Default Case Conversion algorithm
   * and the locale-insensitive case mappings in the Unicode Character Database.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(upcase(get("key-to-string-value")))
   * );
   * }
   * </pre>
   *
   * @param string the string to upcase
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#upcase">Style specification</a>
   */
  public static Expression upcase(@NonNull Expression string) {
    return new Expression("upcase", string);
  }

  /**
   * Returns the input string converted to uppercase.
   * <p>
   * Follows the Unicode Default Case Conversion algorithm
   * and the locale-insensitive case mappings in the Unicode Character Database.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(upcase("text"))
   * );
   * }
   * </pre>
   *
   * @param string string to upcase
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#upcase">Style specification</a>
   */
  public static Expression upcase(@NonNull String string) {
    return upcase(literal(string));
  }

  /**
   * Returns the input string converted to lowercase.
   * <p>
   * Follows the Unicode Default Case Conversion algorithm
   * and the locale-insensitive case mappings in the Unicode Character Database.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(downcase(get("key-to-string-value")))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#downcase">Style specification</a>
   */
  public static Expression downcase(@NonNull Expression input) {
    return new Expression("downcase", input);
  }

  /**
   * Returns the input string converted to lowercase.
   * <p>
   * Follows the Unicode Default Case Conversion algorithm
   * and the locale-insensitive case mappings in the Unicode Character Database.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(upcase("key-to-string-value"))
   * );
   * }
   * </pre>
   *
   * @param input string to downcase
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#downcase">Style specification</a>
   */
  public static Expression downcase(@NonNull String input) {
    return downcase(literal(input));
  }

  /**
   * Returns a string consisting of the concatenation of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(concat(get("key-to-string-value"), literal("other string")))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#concat">Style specification</a>
   */
  public static Expression concat(@NonNull Expression... input) {
    return new Expression("concat", input);
  }

  /**
   * Returns a string consisting of the concatenation of the inputs.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(concat("foo", "bar"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#concat">Style specification</a>
   */
  public static Expression concat(@NonNull String... input) {
    Expression[] stringExpression = new Expression[input.length];
    for (int i = 0; i < input.length; i++) {
      stringExpression[i] = literal(input[i]);
    }
    return concat(stringExpression);
  }

  /**
   * Asserts that the input is an array (optionally with a specific item type and length).
   * If, when the input expression is evaluated, it is not of the asserted type,
   * then this assertion will cause the whole expression to be aborted.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-array">Style specification</a>
   */
  public static Expression array(@NonNull Expression input) {
    return new Expression("array", input);
  }

  /**
   * Returns a string describing the type of the given value.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-typeof">Style specification</a>
   */
  public static Expression typeOf(@NonNull Expression input) {
    return new Expression("typeof", input);
  }

  /**
   * Asserts that the input value is a string.
   * If multiple values are provided, each one is evaluated in order until a string value is obtained.
   * If none of the inputs are strings, the expression is an error.
   * The asserted input value is returned as result.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-string">Style specification</a>
   */
  public static Expression string(@NonNull Expression... input) {
    return new Expression("string", input);
  }

  /**
   * Asserts that the input value is a number.
   * If multiple values are provided, each one is evaluated in order until a number value is obtained.
   * If none of the inputs are numbers, the expression is an error.
   * The asserted input value is returned as result.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-number">Style specification</a>
   */
  public static Expression number(@NonNull Expression... input) {
    return new Expression("number", input);
  }

  /**
   * Converts the input number into a string representation using the providing formatting rules.
   * If set, the locale argument specifies the locale to use, as a BCP 47 language tag.
   * If set, the currency argument specifies an ISO 4217 code to use for currency-style formatting.
   * If set, the min-fraction-digits and max-fraction-digits arguments specify the minimum and maximum number
   * of fractional digits to include.
   *
   * @param number  number expression
   * @param options number formatting options
   * @return expression
   */
  public static Expression numberFormat(@NonNull Expression number, @NonNull NumberFormatOption... options) {
    final Map<String, Expression> map = new HashMap<>();
    for (NumberFormatOption option : options) {
      map.put(option.type, option.value);
    }
    return new Expression("number-format", number, new ExpressionMap(map));
  }

  /**
   * Converts the input number into a string representation using the providing formatting rules.
   * If set, the locale argument specifies the locale to use, as a BCP 47 language tag.
   * If set, the currency argument specifies an ISO 4217 code to use for currency-style formatting.
   * If set, the min-fraction-digits and max-fraction-digits arguments specify the minimum and maximum number
   * of fractional digits to include.
   *
   * @param number  number expression
   * @param options number formatting options
   * @return expression
   */
  public static Expression numberFormat(@NonNull Number number, @NonNull NumberFormatOption... options) {
    return numberFormat(literal(number), options);
  }

  /**
   * Asserts that the input value is a boolean.
   * If multiple values are provided, each one is evaluated in order until a boolean value is obtained.
   * If none of the inputs are booleans, the expression is an error.
   * The asserted input value is returned as result.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-boolean">Style specification</a>
   */
  public static Expression bool(@NonNull Expression... input) {
    return new Expression("boolean", input);
  }

  /**
   * Returns a collator for use in locale-dependent comparison operations.
   * The case-sensitive and diacritic-sensitive options default to false.
   * The locale argument specifies the IETF language tag of the locale to use.
   * If none is provided, the default locale is used. If the requested locale is not available,
   * the collator will use a system-defined fallback locale.
   * Use resolved-locale to test the results of locale fallback behavior.
   *
   * @param caseSensitive      case sensitive flag
   * @param diacriticSensitive diacritic sensitive flag
   * @param locale             locale
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-collator">Style specification</a>
   */
  public static Expression collator(boolean caseSensitive, boolean diacriticSensitive, Locale locale) {
    Map<String, Expression> map = new HashMap<>();
    map.put("case-sensitive", literal(caseSensitive));
    map.put("diacritic-sensitive", literal(diacriticSensitive));

    StringBuilder localeStringBuilder = new StringBuilder();

    String language = locale.getLanguage();
    if (language != null && !language.isEmpty()) {
      localeStringBuilder.append(language);
    }

    String country = locale.getCountry();
    if (country != null && !country.isEmpty()) {
      localeStringBuilder.append("-");
      localeStringBuilder.append(country);
    }

    map.put("locale", literal(localeStringBuilder.toString()));
    return new Expression("collator", new ExpressionMap(map));
  }

  /**
   * Returns a collator for use in locale-dependent comparison operations.
   * The case-sensitive and diacritic-sensitive options default to false.
   * The locale argument specifies the IETF language tag of the locale to use.
   * If none is provided, the default locale is used. If the requested locale is not available,
   * the collator will use a system-defined fallback locale.
   * Use resolved-locale to test the results of locale fallback behavior.
   *
   * @param caseSensitive      case sensitive flag
   * @param diacriticSensitive diacritic sensitive flag
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-collator">Style specification</a>
   */
  public static Expression collator(boolean caseSensitive, boolean diacriticSensitive) {
    Map<String, Expression> map = new HashMap<>();
    map.put("case-sensitive", literal(caseSensitive));
    map.put("diacritic-sensitive", literal(diacriticSensitive));
    return new Expression("collator", new ExpressionMap(map));
  }

  /**
   * Returns a collator for use in locale-dependent comparison operations.
   * The case-sensitive and diacritic-sensitive options default to false.
   * The locale argument specifies the IETF language tag of the locale to use.
   * If none is provided, the default locale is used. If the requested locale is not available,
   * the collator will use a system-defined fallback locale.
   * Use resolved-locale to test the results of locale fallback behavior.
   *
   * @param caseSensitive      case sensitive flag
   * @param diacriticSensitive diacritic sensitive flag
   * @param locale             locale
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-collator">Style specification</a>
   */
  public static Expression collator(Expression caseSensitive, Expression diacriticSensitive, Expression locale) {
    Map<String, Expression> map = new HashMap<>();
    map.put("case-sensitive", caseSensitive);
    map.put("diacritic-sensitive", diacriticSensitive);
    map.put("locale", locale);
    return new Expression("collator", new ExpressionMap(map));
  }

  /**
   * Returns a collator for use in locale-dependent comparison operations.
   * The case-sensitive and diacritic-sensitive options default to false.
   * The locale argument specifies the IETF language tag of the locale to use.
   * If none is provided, the default locale is used. If the requested locale is not available,
   * the collator will use a system-defined fallback locale.
   * Use resolved-locale to test the results of locale fallback behavior.
   *
   * @param caseSensitive      case sensitive flag
   * @param diacriticSensitive diacritic sensitive flag
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-collator">Style specification</a>
   */
  public static Expression collator(Expression caseSensitive, Expression diacriticSensitive) {
    Map<String, Expression> map = new HashMap<>();
    map.put("case-sensitive", caseSensitive);
    map.put("diacritic-sensitive", diacriticSensitive);
    return new Expression("collator", new ExpressionMap(map));
  }

  /**
   * Returns formatted text containing annotations for use in mixed-format text-field entries.
   * <p>
   * To build the expression, use {@link #formatEntry(Expression, FormatOption...)}.
   * <p>
   * "format" expression can be used, for example, with the {@link PropertyFactory#textField(Expression)}
   * and accepts unlimited numbers of formatted sections.
   * <p>
   * Each section consist of the input, the displayed text, and options, like font-scale and text-font.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *   textField(
   *     format(
   *       formatEntry(
   *         get("header_property"),
   *         formatFontScale(2.0),
   *         formatTextFont(new String[] {"DIN Offc Pro Regular", "Arial Unicode MS Regular"})
   *       ),
   *       formatEntry(concat(literal("\n"), get("description_property")), formatFontScale(1.5))
   *     )
   *   )
   * );
   * }
   * </pre>
   *
   * @param formatEntries format entries
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-format">Style specification</a>
   */
  public static Expression format(@NonNull FormatEntry... formatEntries) {
    // for each entry we are going to build an input and parameters
    Expression[] mappedExpressions = new Expression[formatEntries.length * 2];

    int mappedIndex = 0;
    for (FormatEntry formatEntry : formatEntries) {
      // input
      mappedExpressions[mappedIndex++] = formatEntry.text;

      // parameters
      Map<String, Expression> map = new HashMap<>();

      if (formatEntry.options != null) {
        for (FormatOption option : formatEntry.options) {
          map.put(option.type, option.value);
        }
      }

      mappedExpressions[mappedIndex++] = new ExpressionMap(map);
    }

    return new Expression("format", mappedExpressions);
  }

  /**
   * Returns a format entry that can be used in {@link #format(FormatEntry...)} to create formatted text fields.
   * <p>
   * Text is required to be of a resulting type string.
   * <p>
   * Text is required to be passed; {@link FormatOption}s are optional and will default to the base values defined
   * for the symbol.
   *
   * @param text          displayed text
   * @param formatOptions format options
   * @return format entry
   */
  public static FormatEntry formatEntry(@NonNull Expression text, @Nullable FormatOption... formatOptions) {
    return new FormatEntry(text, formatOptions);
  }

  /**
   * Returns a format entry that can be used in {@link #format(FormatEntry...)} to create formatted text fields.
   * <p>
   * Text is required to be of a resulting type string.
   * <p>
   * Text is required to be passed; {@link FormatOption}s are optional and will default to the base values defined
   * for the symbol.
   *
   * @param text displayed text
   * @return format entry
   */
  public static FormatEntry formatEntry(@NonNull Expression text) {
    return new FormatEntry(text, null);
  }

  /**
   * Returns a format entry that can be used in {@link #format(FormatEntry...)} to create formatted text fields.
   * <p>
   * Text is required to be of a resulting type string.
   * <p>
   * Text is required to be passed; {@link FormatOption}s are optional and will default to the base values defined
   * for the symbol.
   *
   * @param text          displayed text
   * @param formatOptions format options
   * @return format entry
   */
  public static FormatEntry formatEntry(@NonNull String text, @Nullable FormatOption... formatOptions) {
    return new FormatEntry(literal(text), formatOptions);
  }

  /**
   * Returns a format entry that can be used in {@link #format(FormatEntry...)} to create formatted text fields.
   * <p>
   * Text is required to be of a resulting type string.
   * <p>
   * Text is required to be passed; {@link FormatOption}s are optional and will default to the base values defined
   * for the symbol.
   *
   * @param text displayed text
   * @return format entry
   */
  public static FormatEntry formatEntry(@NonNull String text) {
    return new FormatEntry(literal(text), null);
  }

  /**
   * Returns image expression for use in '*-pattern' and 'icon-image' layer properties. Compared to
   * string literals that can be used to represent an image, image expression allows to determine an
   * image's availability at runtime, thus, can be used in conditional <a href="https://docs.mapbox.com/mapbox-gl-js/style-spec/#expressions-coalesce">coalesce operator</a>.
   *
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     iconImage(image(get("key-to-feature")))
   * );
   * }
   * </pre>
   *
   * <p>
   * Example usage with coalesce operator:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     iconImage(
   *         coalesce(
   *             image(literal("maki-11")),
   *             image(literal("bicycle-15")),
   *             image(literal("default-icon"))
   *         )
   *     )
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://docs.mapbox.com/mapbox-gl-js/style-spec/#expressions-types-image">Image expression</a>
   */
  public static Expression image(@NonNull Expression input) {
    return new Expression("image", input);
  }

  /**
   * Asserts that the input value is an object. If it is not, the expression is an error
   * The asserted input value is returned as result.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-object">Style specification</a>
   */
  public static Expression object(@NonNull Expression input) {
    return new Expression("object", input);
  }

  /**
   * Converts the input value to a string.
   * If the input is null, the result is null.
   * If the input is a boolean, the result is true or false.
   * If the input is a number, it is converted to a string by NumberToString in the ECMAScript Language Specification.
   * If the input is a color, it is converted to a string of the form "rgba(r,g,b,a)",
   * where `r`, `g`, and `b` are numerals ranging from 0 to 255, and `a` ranges from 0 to 1.
   * Otherwise, the input is converted to a string in the format specified by the JSON.stringify in the ECMAScript
   * Language Specification.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * SymbolLayer symbolLayer = new SymbolLayer("layer-id", "source-id");
   * symbolLayer.setProperties(
   *     textField(get("key-to-number-value"))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-to-string">Style specification</a>
   */
  public static Expression toString(@NonNull Expression input) {
    return new Expression("to-string", input);
  }

  /**
   * Converts the input value to a number, if possible.
   * If the input is null or false, the result is 0.
   * If the input is true, the result is 1.
   * If the input is a string, it is converted to a number as specified by the ECMAScript Language Specification.
   * If multiple values are provided, each one is evaluated in order until the first successful conversion is obtained.
   * If none of the inputs can be converted, the expression is an error.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(toNumber(get("key-to-string-value")))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-to-number">Style specification</a>
   */
  public static Expression toNumber(@NonNull Expression input) {
    return new Expression("to-number", input);
  }

  /**
   * Converts the input value to a boolean. The result is `false` when then input is an empty string, 0, false,
   * null, or NaN; otherwise it is true.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(toBool(get("key-to-value")))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-to-boolean">Style specification</a>
   */
  public static Expression toBool(@NonNull Expression input) {
    return new Expression("to-boolean", input);
  }

  /**
   * Converts the input value to a color. If multiple values are provided,
   * each one is evaluated in order until the first successful conversion is obtained.
   * If none of the inputs can be converted, the expression is an error.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(toColor(get("keyStringValue")))
   * );
   * }
   * </pre>
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#types-to-color">Style specification</a>
   */
  public static Expression toColor(@NonNull Expression input) {
    return new Expression("to-color", input);
  }

  /**
   * Converts input value to a padding.
   *
   * If the input is a number or an array of numbers padding is created following
   * the same pattern as CSS padding. See <a href="https://maplibre.org/maplibre-style-spec/types/#padding">Style specification</a>.
   * Otherwise, the expression is an error.
   *
   * @param input expression input
   * @return expression
   */
  public static Expression toPadding(@NonNull Expression input) {
    return new Expression("to-padding", input);
  }

  /**
   * Binds input to named variables,
   * which can then be referenced in the result expression using {@link #var(String)} or {@link #var(Expression)}.
   *
   * @param input expression input
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#let">Style specification</a>
   */
  public static Expression let(@Size(min = 1) Expression... input) {
    return new Expression("let", input);
  }

  /**
   * References variable bound using let.
   *
   * @param expression the variable naming expression that was bound with using let
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#var">Style specification</a>
   */
  public static Expression var(@NonNull Expression expression) {
    return new Expression("var", expression);
  }

  /**
   * References variable bound using let.
   *
   * @param variableName the variable naming that was bound with using let
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#var">Style specification</a>
   */
  public static Expression var(@NonNull String variableName) {
    return var(literal(variableName));
  }

  /**
   * Gets the current zoom level.
   * <p>
   * Note that in style layout and paint properties,
   * zoom may only appear as the input to a top-level step or interpolate expression.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         exponential(0.5f), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#zoom">Style specification</a>
   */
  public static Expression zoom() {
    return new Expression("zoom");
  }

  /**
   * Produces a stop value.
   * <p>
   * Can be used for {@link #stop(Object, Object)} as part of varargs parameter in
   * {@link #step(Number, Expression, Stop...)} or {@link #interpolate(Interpolator, Expression, Stop...)}.
   * </p>
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), literal(0.0f),
   *         stop(1.0f, 2.5f),
   *         stop(10.0f, 5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param stop  the stop input
   * @param value the stop output
   * @return the stop
   */
  public static Stop stop(@NonNull Object stop, @NonNull Object value) {
    return new Stop(stop, value);
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), literal(0.0f),
   *         literal(1.0f), literal(2.5f),
   *         literal(10.0f), literal(5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Number input, @NonNull Expression defaultOutput, Expression... stops) {
    return step(literal(input), defaultOutput, stops);
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), literal(0.0f),
   *         literal(1.0f), literal(2.5f),
   *         literal(10.0f), literal(5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input expression
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Expression input, @NonNull Expression defaultOutput,
                                @NonNull Expression... stops) {
    return new Expression("step", join(new Expression[] {input, defaultOutput}, stops));
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), literal(0.0f),
   *         stop(1, 2.5f),
   *         stop(10, 5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Number input, @NonNull Expression defaultOutput, Stop... stops) {
    return step(literal(input), defaultOutput, Stop.toExpressionArray(stops));
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), literal(0.0f),
   *         stop(1, 2.5f),
   *         stop(10, 5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Expression input, @NonNull Expression defaultOutput, Stop... stops) {
    return step(input, defaultOutput, Stop.toExpressionArray(stops));
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(1.0f, 0.0f,
   *         literal(1.0f), literal(2.5f),
   *         literal(10.0f), literal(5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Number input, @NonNull Number defaultOutput, Expression... stops) {
    return step(literal(input), defaultOutput, stops);
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), 0.0f,
   *         literal(1.0f), literal(2.5f),
   *         literal(10.0f), literal(5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input expression
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Expression input, @NonNull Number defaultOutput, Expression... stops) {
    return step(input, literal(defaultOutput), stops);
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), 0.0f,
   *         stop(1, 2.5f),
   *         stop(10, 5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Number input, @NonNull Number defaultOutput, Stop... stops) {
    return step(literal(input), defaultOutput, Stop.toExpressionArray(stops));
  }

  /**
   * Produces discrete, stepped results by evaluating a piecewise-constant function defined by pairs of
   * input and output values (\"stops\"). The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * Returns the output value of the stop just less than the input,
   * or the first input if the input is less than the first stop.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * CircleLayer circleLayer = new CircleLayer("layer-id", "source-id");
   * circleLayer.setProperties(
   *     circleRadius(
   *         step(zoom(), 0.0f,
   *         stop(1, 2.5f),
   *         stop(10, 5.0f))
   *     )
   * );
   * }
   * </pre>
   *
   * @param input         the input value
   * @param defaultOutput the default output expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#step">Style specification</a>
   */
  public static Expression step(@NonNull Expression input, @NonNull Number defaultOutput, Stop... stops) {
    return step(input, defaultOutput, Stop.toExpressionArray(stops));
  }

  /**
   * Produces continuous, smooth results by interpolating between pairs of input and output values (\"stops\").
   * The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * The output type must be `number`, `array&lt;number&gt;`, or `color`.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *   fillColor(
   *     interpolate(exponential(0.5f), zoom(),
   *        stop(1.0f, color(Color.RED)),
   *        stop(5.0f, color(Color.BLUE)),
   *        stop(10.0f, color(Color.GREEN)
   *       )
   *     )
   *   )
   * );
   * }
   * </pre>
   *
   * @param interpolation type of interpolation
   * @param number        the input expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Expression interpolate(@NonNull Interpolator interpolation,
                                       @NonNull Expression number, @NonNull Expression... stops) {
    return new Expression("interpolate", join(new Expression[] {interpolation, number}, stops));
  }

  /**
   * Produces continuous, smooth results by interpolating between pairs of input and output values (\"stops\").
   * The `input` may be any numeric expression (e.g., `[\"get\", \"population\"]`).
   * Stop inputs must be numeric literals in strictly ascending order.
   * The output type must be `number`, `array&lt;number&gt;`, or `color`.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         exponential(0.5f), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @param interpolation type of interpolation
   * @param number        the input expression
   * @param stops         pair of input and output values
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Expression interpolate(@NonNull Interpolator interpolation,
                                       @NonNull Expression number, Stop... stops) {
    return interpolate(interpolation, number, Stop.toExpressionArray(stops));
  }

  /**
   * interpolates linearly between the pair of stops just less than and just greater than the input.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         linear(), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Interpolator linear() {
    return new Interpolator("linear");
  }

  /**
   * Interpolates exponentially between the stops just less than and just greater than the input.
   * `base` controls the rate at which the output increases:
   * higher values make the output increase more towards the high end of the range.
   * With values close to 1 the output increases linearly.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         exponential(0.5f), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @param base value controlling the route at which the output increases
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Interpolator exponential(@NonNull Number base) {
    return exponential(literal(base));
  }

  /**
   * Interpolates exponentially between the stops just less than and just greater than the input.
   * The parameter controls the rate at which the output increases:
   * higher values make the output increase more towards the high end of the range.
   * With values close to 1 the output increases linearly.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         exponential(get("keyToValue")), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @param expression base number expression
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Interpolator exponential(@NonNull Expression expression) {
    return new Interpolator("exponential", expression);
  }

  /**
   * Interpolates using the cubic bezier curve defined by the given control points.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         cubicBezier(0.42f, 0.0f, 1.0f, 1.0f), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @param x1 x value of the first point of a cubic bezier, ranges from 0 to 1
   * @param y1 y value of the first point of a cubic bezier, ranges from 0 to 1
   * @param x2 x value of the second point of a cubic bezier, ranges from 0 to 1
   * @param y2 y value fo the second point of a cubic bezier, ranges from 0 to 1
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Interpolator cubicBezier(@NonNull Expression x1, @NonNull Expression y1,
                                         @NonNull Expression x2, @NonNull Expression y2) {
    return new Interpolator("cubic-bezier", x1, y1, x2, y2);
  }

  /**
   * Interpolates using the cubic bezier curve defined by the given control points.
   * <p>
   * Example usage:
   * </p>
   * <pre>
   * {@code
   * FillLayer fillLayer = new FillLayer("layer-id", "source-id");
   * fillLayer.setProperties(
   *     fillColor(
   *       interpolate(
   *         cubicBezier(0.42f, 0.0f, 1.0f, 1.0f), zoom(),
   *         stop(1.0f, color(Color.RED)),
   *         stop(5.0f, color(Color.BLUE)),
   *         stop(10.0f, color(Color.GREEN))
   *       )
   *     )
   * );
   * }
   * </pre>
   *
   * @param x1 x value of the first point of a cubic bezier, ranges from 0 to 1
   * @param y1 y value of the first point of a cubic bezier, ranges from 0 to 1
   * @param x2 x value of the second point of a cubic bezier, ranges from 0 to 1
   * @param y2 y value fo the second point of a cubic bezier, ranges from 0 to 1
   * @return expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/expressions/#interpolate">Style specification</a>
   */
  public static Interpolator cubicBezier(@NonNull Number x1, @NonNull Number y1,
                                         @NonNull Number x2, @NonNull Number y2) {
    return cubicBezier(literal(x1), literal(y1), literal(x2), literal(y2));
  }

  /**
   * Joins two expressions arrays.
   * <p>
   * This flattens the object array output of an expression from a nested expression hierarchy.
   * </p>
   *
   * @param left  the left part of an expression
   * @param right the right part of an expression
   * @return the joined expression
   */
  @NonNull
  private static Expression[] join(Expression[] left, Expression[] right) {
    Expression[] output = new Expression[left.length + right.length];
    System.arraycopy(left, 0, output, 0, left.length);
    System.arraycopy(right, 0, output, left.length, right.length);
    return output;
  }

  /**
   * Converts the expression to Object array representation.
   * <p>
   * The output will later be converted to a JSON Object array.
   * </p>
   *
   * @return the converted object array expression
   */
  @NonNull
  public Object[] toArray() {
    List<Object> array = new ArrayList<>();
    array.add(operator);
    if (arguments != null) {
      for (Expression argument : arguments) {
        if (argument instanceof ValueExpression) {
          array.add(((ValueExpression) argument).toValue());
        } else {
          array.add(argument.toArray());
        }
      }
    }
    return array.toArray();
  }

  /**
   * Returns a string representation of the object that matches the definition set in the style specification.
   * <p>
   * If this expression contains a coma (,) delimited literal, like 'rgba(r, g, b, a)`,
   * it will be enclosed with double quotes (").
   * </p>
   *
   * @return a string representation of the object.
   */
  @Override
  public String toString() {
    StringBuilder builder = new StringBuilder();
    builder.append("[\"").append(operator).append("\"");
    if (arguments != null) {
      for (Object argument : arguments) {
        builder.append(", ");
        builder.append(argument.toString());
      }
    }
    builder.append("]");
    return builder.toString();
  }

  /**
   * Returns a DSL equivalent of a raw expression.
   * <p>
   * If your raw expression contains a coma (,) delimited literal it has to be enclosed with double quotes ("),
   * for example
   * </p>
   * <pre>
   *   {@code
   *   ["to-color", "rgba(255, 0, 0, 255)"]
   *   }
   * </pre>
   *
   * @param rawExpression the raw expression
   * @return the resulting expression
   * @see <a href="https://maplibre.org/maplibre-style-spec/">Style specification</a>
   */
  public static Expression raw(@NonNull String rawExpression) {
    return Converter.convert(rawExpression);
  }

  /**
   * Indicates whether some other object is "equal to" this one.
   *
   * @param o the other object
   * @return true if equal, false if not
   */
  @Override
  public boolean equals(@Nullable Object o) {
    super.equals(o);
    if (this == o) {
      return true;
    }

    if (o == null || !(o instanceof Expression)) {
      return false;
    }

    Expression that = (Expression) o;

    if (operator != null ? !operator.equals(that.operator) : that.operator != null) {
      return false;
    }
    return Arrays.deepEquals(arguments, that.arguments);
  }

  /**
   * Returns a hash code value for the expression.
   *
   * @return a hash code value for this expression
   */
  @Override
  public int hashCode() {
    int result = operator != null ? operator.hashCode() : 0;
    result = 31 * result + Arrays.hashCode(arguments);
    return result;
  }

  /**
   * ExpressionLiteral wraps an object to be used as a literal in an expression.
   * <p>
   * ExpressionLiteral is created with {@link #literal(Number)}, {@link #literal(boolean)},
   * {@link #literal(String)} and {@link #literal(Object)}.
   * </p>
   */
  public static class ExpressionLiteral extends Expression implements ValueExpression {

    protected Object literal;

    /**
     * Create an expression literal.
     *
     * @param object the object to be treated as literal
     */
    public ExpressionLiteral(@NonNull Object object) {
      if (object instanceof String) {
        object = unwrapStringLiteral((String) object);
      } else if (object instanceof Number) {
        object = ((Number) object).floatValue();
      }
      this.literal = object;
    }

    /**
     * Get the literal object.
     *
     * @return the literal object
     */
    @Override
    public Object toValue() {
      if (literal instanceof PropertyValue) {
        throw new IllegalArgumentException(
          "PropertyValue are not allowed as an expression literal, use value instead.");
      } else if (literal instanceof Expression.ExpressionLiteral) {
        return ((ExpressionLiteral) literal).toValue();
      }
      return literal;
    }

    @NonNull
    @Override
    public Object[] toArray() {
      return new Object[] {"literal", literal};
    }

    /**
     * Returns a string representation of the expression literal.
     *
     * @return a string representation of the object.
     */
    @Override
    public String toString() {
      String string;
      if (literal instanceof String) {
        string = "\"" + literal + "\"";
      } else {
        string = literal.toString();
      }
      return string;
    }

    /**
     * Indicates whether some other object is "equal to" this one.
     *
     * @param o the other object
     * @return true if equal, false if not
     */
    @Override
    public boolean equals(@Nullable Object o) {
      if (this == o) {
        return true;
      }
      if (o == null || getClass() != o.getClass()) {
        return false;
      }
      if (!super.equals(o)) {
        return false;
      }

      ExpressionLiteral that = (ExpressionLiteral) o;

      return literal != null ? literal.equals(that.literal) : that.literal == null;
    }

    /**
     * Returns a hash code value for the expression literal.
     *
     * @return a hash code value for this expression literal
     */
    @Override
    public int hashCode() {
      int result = super.hashCode();
      result = 31 * result + (literal != null ? literal.hashCode() : 0);
      return result;
    }

    @NonNull
    private static String unwrapStringLiteral(String value) {
      if (value.length() > 1 &&
        value.charAt(0) == '\"' && value.charAt(value.length() - 1) == '\"') {
        return value.substring(1, value.length() - 1);
      } else {
        return value;
      }
    }
  }

  /**
   * Expression interpolator type.
   * <p>
   * Is used for first parameter of {@link #interpolate(Interpolator, Expression, Stop...)}.
   * </p>
   */
  public static class Interpolator extends Expression {

    Interpolator(@NonNull String operator, @Nullable Expression... arguments) {
      super(operator, arguments);
    }
  }

  /**
   * Expression array type.
   */
  public static class Array {
  }

  /**
   * Expression stop type.
   * <p>
   * Can be used for {@link #stop(Object, Object)} as part of varargs parameter in
   * {@link #step(Number, Expression, Stop...)} or {@link #interpolate(Interpolator, Expression, Stop...)}.
   * </p>
   */
  public static class Stop {

    private Object value;
    private Object output;

    Stop(Object value, Object output) {
      this.value = value;
      this.output = output;
    }

    /**
     * Converts a varargs of Stops to a Expression array.
     *
     * @param stops the stops to convert
     * @return the converted stops as an expression array
     */
    @NonNull
    static Expression[] toExpressionArray(Stop... stops) {
      Expression[] expressions = new Expression[stops.length * 2];
      Stop stop;
      Object inputValue, outputValue;
      for (int i = 0; i < stops.length; i++) {
        stop = stops[i];
        inputValue = stop.value;
        outputValue = stop.output;

        if (!(inputValue instanceof Expression)) {
          inputValue = literal(inputValue);
        }

        if (!(outputValue instanceof Expression)) {
          outputValue = literal(outputValue);
        }

        expressions[i * 2] = (Expression) inputValue;
        expressions[i * 2 + 1] = (Expression) outputValue;
      }
      return expressions;
    }
  }

  /**
   * Holds format entries used in a {@link #format(FormatEntry...)} expression.
   */
  public static class FormatEntry {
    @NonNull
    private Expression text;
    @Nullable
    private FormatOption[] options;

    FormatEntry(@NonNull Expression text, @Nullable FormatOption[] options) {
      this.text = text;
      this.options = options;
    }
  }

  /**
   * Base class for an option entry that is encapsulated as a json object member for an expression.
   */
  private static class Option {
    @NonNull
    String type;
    @NonNull
    Expression value;

    /**
     * Create an option option entry that is encapsulated as a json object member for an expression.
     *
     * @param type  json object member name
     * @param value json object member value
     */
    Option(@NonNull String type, @NonNull Expression value) {
      this.type = type;
      this.value = value;
    }
  }

  /**
   * Holds format options used in a {@link #numberFormat(Number, NumberFormatOption...)} expression.
   */
  public static class NumberFormatOption extends Option {

    /**
     * {@inheritDoc}
     */
    NumberFormatOption(@NonNull String type, @NonNull Expression value) {
      super(type, value);
    }

    /**
     * Number formatting option for specifying the locale to use, as a BCP 47 language tag.
     *
     * @param string the locale to use while performing number formatting
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption locale(@NonNull Expression string) {
      return new NumberFormatOption("locale", string);
    }

    /**
     * Number formatting option for specifying the locale to use, as a BCP 47 language tag.
     *
     * @param string the locale to use while performing number formatting
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption locale(@NonNull String string) {
      return new NumberFormatOption("locale", literal(string));
    }

    /**
     * Number formatting option for specifying the currency to use, an ISO 4217 code.
     *
     * @param string the currency to use while performing number formatting
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption currency(@NonNull Expression string) {
      return new NumberFormatOption("currency", string);
    }

    /**
     * Number formatting options for specifying the currency to use, an ISO 4217 code.
     *
     * @param string the currency to use while performing number formatting
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption currency(@NonNull String string) {
      return new NumberFormatOption("currency", literal(string));
    }

    /**
     * Number formatting options for specifying the minimum fraction digits to include.
     *
     * @param number the amount of minimum fraction digits to include
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption minFractionDigits(@NonNull Expression number) {
      return new NumberFormatOption("min-fraction-digits", number);
    }

    /**
     * Number formatting options for specifying the minimum fraction digits to include.
     *
     * @param number the amount of minimum fraction digits to include
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption minFractionDigits(int number) {
      return new NumberFormatOption("min-fraction-digits", literal(number));
    }

    /**
     * Number formatting options for specifying the maximum fraction digits to include.
     *
     * @param number the amount of minimum fraction digits to include
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption maxFractionDigits(@NonNull Expression number) {
      return new NumberFormatOption("max-fraction-digits", number);
    }

    /**
     * Number formatting options for specifying the maximum fraction digits to include.
     *
     * @param number the amount of minimum fraction digits to include
     * @return number format option
     */
    @NonNull
    public static NumberFormatOption maxFractionDigits(@NonNull int number) {
      return new NumberFormatOption("max-fraction-digits", literal(number));
    }
  }

  /**
   * Holds format options used in a {@link #formatEntry(Expression, FormatOption...)} that builds
   * a {@link #format(FormatEntry...)} expression.
   * <p>
   * If an option is not set, it defaults to the base value defined for the symbol.
   */
  public static class FormatOption extends Option {

    FormatOption(@NonNull String type, @NonNull Expression value) {
      super(type, value);
    }

    /**
     * If set, the font-scale argument specifies a scaling factor relative to the text-size
     * specified in the root layout properties.
     * <p>
     * "font-scale" is required to be of a resulting type number.
     *
     * @param expression expression
     * @return format option
     */
    @NonNull
    public static FormatOption formatFontScale(@NonNull Expression expression) {
      return new FormatOption("font-scale", expression);
    }

    /**
     * If set, the font-scale argument specifies a scaling factor relative to the text-size
     * specified in the root layout properties.
     * <p>
     * "font-scale" is required to be of a resulting type number.
     *
     * @param scale value
     * @return format option
     */
    @NonNull
    public static FormatOption formatFontScale(double scale) {
      return new FormatOption("font-scale", literal(scale));
    }

    /**
     * If set, the text-font argument overrides the font specified by the root layout properties.
     * <p>
     * "text-font" is required to be a literal array.
     * <p>
     * The requested font stack has to be a part of the used style.
     * For more information see <a href="https://www.mapbox.com/help/define-font-stack/">the documentation</a>.
     *
     * @param expression expression
     * @return format option
     */
    @NonNull
    public static FormatOption formatTextFont(@NonNull Expression expression) {
      return new FormatOption("text-font", expression);
    }

    /**
     * If set, the text-font argument overrides the font specified by the root layout properties.
     * <p>
     * "text-font" is required to be a literal array.
     * <p>
     * The requested font stack has to be a part of the used style.
     * For more information see <a href="https://www.mapbox.com/help/define-font-stack/">the documentation</a>.
     *
     * @param fontStack value
     * @return format option
     */
    @NonNull
    public static FormatOption formatTextFont(@NonNull String[] fontStack) {
      return new FormatOption("text-font", literal(fontStack));
    }

    /**
     * If set, the text-color argument overrides the color specified by the root paint properties.
     *
     * @param expression expression
     * @return format option
     */
    @NonNull
    public static FormatOption formatTextColor(@NonNull Expression expression) {
      return new FormatOption("text-color", expression);
    }

    /**
     * If set, the text-color argument overrides the color specified by the root paint properties.
     *
     * @param color value
     * @return format option
     */
    @NonNull
    public static FormatOption formatTextColor(@ColorInt int color) {
      return new FormatOption("text-color", color(color));
    }
  }

  /**
   * Converts a JsonArray or a raw expression to a Java expression.
   */
  public final static class Converter {

    private static final Gson gson = new Gson();

    /**
     * Converts a JsonArray to an expression
     *
     * @param jsonArray the json array to convert
     * @return the expression
     */
    public static Expression convert(@NonNull JsonArray jsonArray) {
      if (jsonArray.size() == 0) {
        throw new IllegalArgumentException("Can't convert empty jsonArray expressions");
      }

      final String operator = jsonArray.get(0).getAsString();
      final List<Expression> arguments = new ArrayList<>();
      if (operator.equals("within")) {
        return within(Polygon.fromJson(jsonArray.get(1).toString()));
      } else if (operator.equals("distance")) {
        return distance(GeometryGeoJson.fromJson(jsonArray.get(1).toString()));
      }
      for (int i = 1; i < jsonArray.size(); i++) {
        JsonElement jsonElement = jsonArray.get(i);
        if (operator.equals("literal") && jsonElement instanceof JsonArray) {
          JsonArray nestedArray = (JsonArray) jsonElement;
          Object[] array = new Object[nestedArray.size()];
          for (int j = 0; j < nestedArray.size(); j++) {
            JsonElement element = nestedArray.get(j);
            if (element instanceof JsonPrimitive) {
              array[j] = convertToValue((JsonPrimitive) element);
            } else {
              throw new IllegalArgumentException("Nested literal arrays are not supported.");
            }
          }

          arguments.add(new ExpressionLiteralArray(array));
        } else {
          arguments.add(convert(jsonElement));
        }
      }
      return new Expression(operator, arguments.toArray(new Expression[arguments.size()]));
    }

    /**
     * Converts a JsonElement to an expression
     *
     * @param jsonElement the json element to convert
     * @return the expression
     */
    public static Expression convert(@NonNull JsonElement jsonElement) {
      if (jsonElement instanceof JsonArray) {
        return convert((JsonArray) jsonElement);
      } else if (jsonElement instanceof JsonPrimitive) {
        return convert((JsonPrimitive) jsonElement);
      } else if (jsonElement instanceof JsonNull) {
        return new Expression.ExpressionLiteral("");
      } else if (jsonElement instanceof JsonObject) {
        Map<String, Expression> map = new HashMap<>();
        for (String key : ((JsonObject) jsonElement).keySet()) {
          map.put(key, convert(((JsonObject) jsonElement).get(key)));
        }
        return new ExpressionMap(map);
      } else {
        throw new RuntimeException("Unsupported expression conversion for " + jsonElement.getClass());
      }
    }

    /**
     * Converts a JsonPrimitive to an expression literal
     *
     * @param jsonPrimitive the json primitive to convert
     * @return the expression literal
     */
    private static Expression convert(@NonNull JsonPrimitive jsonPrimitive) {
      return new ExpressionLiteral(convertToValue(jsonPrimitive));
    }

    /**
     * Converts a JsonPrimitive to value
     *
     * @param jsonPrimitive the json primitive to convert
     * @return the value
     */
    private static Object convertToValue(@NonNull JsonPrimitive jsonPrimitive) {
      if (jsonPrimitive.isBoolean()) {
        return jsonPrimitive.getAsBoolean();
      } else if (jsonPrimitive.isNumber()) {
        return jsonPrimitive.getAsFloat();
      } else if (jsonPrimitive.isString()) {
        return jsonPrimitive.getAsString();
      } else {
        throw new RuntimeException("Unsupported literal expression conversion for " + jsonPrimitive.getClass());
      }
    }

    /**
     * Converts a raw expression to a DSL equivalent.
     *
     * @param rawExpression the raw expression to convert
     * @return the resulting expression
     * @see <a href="https://maplibre.org/maplibre-style-spec/">Style specification</a>
     */
    public static Expression convert(@NonNull String rawExpression) {
      return convert(gson.fromJson(rawExpression, JsonArray.class));
    }
  }

  /**
   * Expression to wrap Object[] as a literal
   */
  private static class ExpressionLiteralArray extends ExpressionLiteral {

    /**
     * Create an expression literal.
     *
     * @param object the object to be treated as literal
     */
    ExpressionLiteralArray(@NonNull Object[] object) {
      super(object);
    }

    /**
     * Convert the expression array to a string representation.
     *
     * @return the string representation of the expression array
     */
    @NonNull
    @Override
    public String toString() {
      Object[] array = (Object[]) literal;
      StringBuilder builder = new StringBuilder("[");
      for (int i = 0; i < array.length; i++) {
        Object argument = array[i];
        if (argument instanceof String) {
          builder.append("\"").append(argument).append("\"");
        } else {
          builder.append(argument);
        }

        if (i != array.length - 1) {
          builder.append(", ");
        }
      }
      builder.append("]");
      return builder.toString();
    }

    @Override
    public boolean equals(@Nullable Object o) {
      if (this == o) {
        return true;
      }
      if (o == null || getClass() != o.getClass()) {
        return false;
      }

      ExpressionLiteralArray that = (ExpressionLiteralArray) o;

      return Arrays.equals((Object[]) this.literal, (Object[]) that.literal);
    }
  }

  /**
   * Wraps an expression value stored in a Map.
   */
  private static class ExpressionMap extends Expression implements ValueExpression {
    private Map<String, Expression> map;

    ExpressionMap(Map<String, Expression> map) {
      this.map = map;
    }

    @NonNull
    @Override
    public Object toValue() {
      Map<String, Object> unwrappedMap = new HashMap<>();
      for (String key : map.keySet()) {
        Expression expression = map.get(key);
        if (expression instanceof ValueExpression) {
          unwrappedMap.put(key, ((ValueExpression) expression).toValue());
        } else {
          unwrappedMap.put(key, expression.toArray());
        }
      }

      return unwrappedMap;
    }

    @NonNull
    @Override
    public String toString() {
      StringBuilder builder = new StringBuilder();
      builder.append("{");
      for (String key : map.keySet()) {
        builder.append("\"").append(key).append("\": ");
        builder.append(map.get(key));
        builder.append(", ");
      }

      if (map.size() > 0) {
        builder.delete(builder.length() - 2, builder.length());
      }

      builder.append("}");
      return builder.toString();
    }

    @Override
    public boolean equals(@Nullable Object o) {
      if (this == o) {
        return true;
      }
      if (o == null || getClass() != o.getClass()) {
        return false;
      }
      if (!super.equals(o)) {
        return false;
      }
      ExpressionMap that = (ExpressionMap) o;
      return map.equals(that.map);
    }

    @Override
    public int hashCode() {
      int result = super.hashCode();
      result = 31 * result + (map == null ? 0 : map.hashCode());
      return result;
    }
  }

  /**
   * Interface used to describe expressions that hold a Java value.
   */
  private interface ValueExpression {
    Object toValue();
  }

  /**
   * Converts an object that is a primitive array to an Object[]
   *
   * @param object the object to convert to an object array
   * @return the converted object array
   */
  @NonNull
  private static Object[] toObjectArray(Object object) {
    // object is a primitive array
    int len = java.lang.reflect.Array.getLength(object);
    Object[] objects = new Object[len];
    for (int i = 0; i < len; i++) {
      objects[i] = java.lang.reflect.Array.get(object, i);
    }
    return objects;
  }
}
