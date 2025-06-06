#import "MLNFoundation_Private.h"
#import "MLNGeometry_Private.h"
#import "MLNShape_Private.h"
#import "NSExpression+MLNPrivateAdditions.h"

#import "MLNTypes.h"
#if TARGET_OS_IPHONE
    #import "UIColor+MLNAdditions.h"
#else
    #import "NSColor+MLNAdditions.h"
#endif
#import "NSPredicate+MLNAdditions.h"
#import "NSValue+MLNStyleAttributeAdditions.h"
#import "MLNVectorTileSource_Private.h"
#import "MLNAttributedExpression.h"

#import <objc/runtime.h>

#import <mbgl/style/expression/expression.hpp>

const MLNExpressionInterpolationMode MLNExpressionInterpolationModeLinear = @"linear";
const MLNExpressionInterpolationMode MLNExpressionInterpolationModeExponential = @"exponential";
const MLNExpressionInterpolationMode MLNExpressionInterpolationModeCubicBezier = @"cubic-bezier";

@interface MLNAftermarketExpressionInstaller: NSObject
@end

@implementation MLNAftermarketExpressionInstaller

+ (void)load {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [self installFunctions];
    });
}

/**
 Adds to NSExpression’s built-in repertoire of functions.
 */
+ (void)installFunctions {
    Class MLNAftermarketExpressionInstaller = [self class];

    // NSExpression’s built-in functions are backed by class methods on a
    // private class, so use a function expression to get at the class.
    // http://funwithobjc.tumblr.com/post/2922267976/using-custom-functions-with-nsexpression
    NSExpression *functionExpression = [NSExpression expressionWithFormat:@"sum({})"];
    NSString *className = NSStringFromClass([functionExpression.operand.constantValue class]);

    // Effectively categorize the class with some extra class methods.
    Class NSPredicateUtilities = objc_getMetaClass(className.UTF8String);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
    #define INSTALL_METHOD(sel) \
        { \
            Method method = class_getInstanceMethod(MLNAftermarketExpressionInstaller, @selector(sel)); \
            class_addMethod(NSPredicateUtilities, @selector(sel), method_getImplementation(method), method_getTypeEncoding(method)); \
        }
    #define INSTALL_CONTROL_STRUCTURE(sel) \
        { \
            Method method = class_getInstanceMethod(MLNAftermarketExpressionInstaller, @selector(sel:)); \
            class_addMethod(NSPredicateUtilities, @selector(sel), method_getImplementation(method), method_getTypeEncoding(method)); \
            class_addMethod(NSPredicateUtilities, @selector(sel:), method_getImplementation(method), method_getTypeEncoding(method)); \
        }

    // Install method-like functions, taking the number of arguments implied by
    // the selector name.
    INSTALL_METHOD(mgl_join:);
    INSTALL_METHOD(mgl_round:);
    INSTALL_METHOD(mgl_interpolate:withCurveType:parameters:stops:);
    INSTALL_METHOD(mgl_step:from:stops:);
    INSTALL_METHOD(mgl_coalesce:);
    INSTALL_METHOD(mgl_does:have:);
    INSTALL_METHOD(mgl_acos:);
    INSTALL_METHOD(mgl_cos:);
    INSTALL_METHOD(mgl_asin:);
    INSTALL_METHOD(mgl_sin:);
    INSTALL_METHOD(mgl_atan:);
    INSTALL_METHOD(mgl_tan:);
    INSTALL_METHOD(mgl_log2:);
    INSTALL_METHOD(mgl_distanceFrom:);
    INSTALL_METHOD(mgl_attributed:);

    // Install functions that resemble control structures, taking arbitrary
    // numbers of arguments. Vararg aftermarket functions need to be declared
    // with an explicit and implicit first argument.
    INSTALL_CONTROL_STRUCTURE(MLN_LET);
    INSTALL_CONTROL_STRUCTURE(MLN_MATCH);
    INSTALL_CONTROL_STRUCTURE(MLN_IF);
    INSTALL_CONTROL_STRUCTURE(MLN_FUNCTION);

    #undef INSTALL_AFTERMARKET_FN
#pragma clang diagnostic pop
}

/**
 Joins the given components into a single string by concatenating each component
 in order.
 */
- (NSString *)mgl_join:(NSArray<NSString *> *)components {
    return [components componentsJoinedByString:@""];
}

- (NSString *)mgl_attributed:(NSArray<MLNAttributedExpression *> *)attributedExpressions {
    [NSException raise:NSInvalidArgumentException
                format:@"Text format expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 Rounds the given number to the nearest integer. If the number is halfway
 between two integers, this method rounds it away from zero.
 */
- (NSNumber *)mgl_round:(NSNumber *)number {
    return @(round(number.doubleValue));
}

/**
  Computes the principal value of the inverse cosine.
 */
- (NSNumber *)mgl_acos:(NSNumber *)number {
    return @(acos(number.doubleValue));
}

/**
 Computes the principal value of the cosine.
 */
- (NSNumber *)mgl_cos:(NSNumber *)number {
    return @(cos(number.doubleValue));
}

/**
 Computes the principal value of the inverse sine.
 */
- (NSNumber *)mgl_asin:(NSNumber *)number {
    return @(asin(number.doubleValue));
}

/**
 Computes the principal value of the sine.
 */
- (NSNumber *)mgl_sin:(NSNumber *)number {
    return @(sin(number.doubleValue));
}

/**
 Computes the principal value of the inverse tangent.
 */
- (NSNumber *)mgl_atan:(NSNumber *)number {
    return @(atan(number.doubleValue));
}

/**
 Computes the principal value of the tangent.
 */
- (NSNumber *)mgl_tan:(NSNumber *)number {
    return @(tan(number.doubleValue));
}

/**
 Computes the logarithm base two of the value.
 */
- (NSNumber *)mgl_log2:(NSNumber *)number {
    return @(log2(number.doubleValue));
}

/**
 Returns a straight-line distance from the given shape to the evaluated feature.
 */
- (NSNumber *)mgl_distanceFrom:(id)object {
    [NSException raise:NSInvalidArgumentException
                format:@"Shape distance expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 A placeholder for a method that evaluates an interpolation expression.
 */
- (id)mgl_interpolate:(id)inputExpression withCurveType:(NSString *)curveType parameters:(NSDictionary *)params stops:(NSDictionary *)stops {
    [NSException raise:NSInvalidArgumentException
                format:@"Interpolation expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 A placeholder for a method that evaluates a step expression.
 */
- (id)mgl_step:(id)inputExpression from:(id)minimumExpression stops:(NSDictionary *)stops {
    [NSException raise:NSInvalidArgumentException
                format:@"Step expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 A placeholder for a method that evaluates a coalesce expression.
 */
- (id)mgl_coalesce:(NSArray<NSExpression *> *)elements {
    [NSException raise:NSInvalidArgumentException
                format:@"Coalesce expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 Returns a Boolean value indicating whether the object has a value for the given
 key.
 */
- (BOOL)mgl_does:(id)object have:(NSString *)key {
    return [object valueForKey:key] != nil;
}

/**
 A placeholder for a method that evaluates an expression based on an arbitrary
 number of variable names and assigned expressions.
 */
- (id)MLN_LET:(NSString *)firstVariableName, ... {
    [NSException raise:NSInvalidArgumentException
                format:@"Assignment expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 A placeholder for a method that evaluates an expression and returns the matching element.
 */
- (id)MLN_MATCH:(id)firstCondition, ... {
    [NSException raise:NSInvalidArgumentException
                format:@"Assignment expressions lack underlying Objective-C implementations."];
    return nil;
}

/**
 A placeholder for a method that evaluates an expression and returns the matching element.
 */
- (id)MLN_IF:(id)firstCondition, ... {
    va_list argumentList;
    va_start(argumentList, firstCondition);

    for (id eachExpression = firstCondition; eachExpression; eachExpression = va_arg(argumentList, id)) {
        if ([eachExpression isKindOfClass:[NSComparisonPredicate class]]) {
            id valueExpression = va_arg(argumentList, id);
            if ([eachExpression evaluateWithObject:nil]) {
                return valueExpression;
            }
        } else {
            return eachExpression;
        }
    }
    va_end(argumentList);

    return nil;
}

/**
 A placeholder for a catch-all method that evaluates an arbitrary number of
 arguments as an expression according to the MapLibre Style Spec’s
 expression language.
 */
- (id)MLN_FUNCTION:(id)firstArgument, ... {
    [NSException raise:NSInvalidArgumentException
                format:@"Mapbox GL function expressions lack underlying Objective-C implementations."];
    return nil;
}

@end

@implementation NSExpression (MLNPrivateAdditions)

- (std::vector<mbgl::Value>)mgl_aggregateMBGLValue {
    if ([self.constantValue isKindOfClass:[NSArray class]] || [self.constantValue isKindOfClass:[NSSet class]]) {
        std::vector<mbgl::Value> convertedValues;
        for (id value in self.constantValue) {
            NSExpression *expression = value;
            if (![expression isKindOfClass:[NSExpression class]]) {
                expression = [NSExpression expressionForConstantValue:expression];
            }
            convertedValues.push_back(expression.mgl_constantMBGLValue);
        }
        return convertedValues;
    }
    [NSException raise:NSInvalidArgumentException
                format:@"Constant value expression must contain an array or set."];
    return {};
}

- (mbgl::Value)mgl_constantMBGLValue {
    id value = self.constantValue;
    if ([value isKindOfClass:NSString.class]) {
        return { std::string([(NSString *)value UTF8String]) };
    } else if ([value isKindOfClass:NSNumber.class]) {
        NSNumber *number = (NSNumber *)value;
        if ((strcmp([number objCType], @encode(char)) == 0) ||
            (strcmp([number objCType], @encode(BOOL)) == 0)) {
            // char: 32-bit boolean
            // BOOL: 64-bit boolean
            return { (bool)number.boolValue };
        } else if (strcmp([number objCType], @encode(double)) == 0) {
            // Double values on all platforms are interpreted precisely.
            return { (double)number.doubleValue };
        } else if (strcmp([number objCType], @encode(float)) == 0) {
            // Float values when taken as double introduce precision problems,
            // so warn the user to avoid them. This would require them to
            // explicitly use -[NSNumber numberWithFloat:] arguments anyway.
            // We still do this conversion in order to provide a valid value.
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                NSLog(@"Float value in expression will be converted to a double; some imprecision may result. "
                      @"Use double values explicitly when specifying constant expression values and "
                      @"when specifying arguments to predicate and expression format strings. "
                      @"This will be logged only once.");
            });
            return { (double)number.doubleValue };
        } else if ([number compare:@(0)] == NSOrderedDescending ||
                   [number compare:@(0)] == NSOrderedSame) {
            // Positive integer or zero; use uint64_t per mbgl::Value definition.
            // We use unsigned long long here to avoid any truncation.
            return { (uint64_t)number.unsignedLongLongValue };
        } else if ([number compare:@(0)] == NSOrderedAscending) {
            // Negative integer; use int64_t per mbgl::Value definition.
            // We use long long here to avoid any truncation.
            return { (int64_t)number.longLongValue };
        }
    } else if ([value isKindOfClass:[MLNColor class]]) {
        auto hexString = [(MLNColor *)value mgl_colorForPremultipliedValue].stringify();
        return { hexString };
    } else if (value && value != [NSNull null]) {
        [NSException raise:NSInvalidArgumentException
                    format:@"Can’t convert %s:%@ to mbgl::Value", [value objCType], value];
    }
    return {};
}

- (std::vector<mbgl::FeatureType>)mgl_aggregateFeatureType {
    if ([self.constantValue isKindOfClass:[NSArray class]] || [self.constantValue isKindOfClass:[NSSet class]]) {
        std::vector<mbgl::FeatureType> convertedValues;
        for (id value in self.constantValue) {
            NSExpression *expression = value;
            if (![expression isKindOfClass:[NSExpression class]]) {
                expression = [NSExpression expressionForConstantValue:expression];
            }
            convertedValues.push_back(expression.mgl_featureType);
        }
        return convertedValues;
    }
    [NSException raise:NSInvalidArgumentException
                format:@"Constant value expression must contain an array or set."];
    return {};
}

- (mbgl::FeatureType)mgl_featureType {
    id value = self.constantValue;
    if ([value isKindOfClass:NSString.class]) {
        if ([value isEqualToString:@"Point"]) {
            return mbgl::FeatureType::Point;
        }
        if ([value isEqualToString:@"LineString"]) {
            return mbgl::FeatureType::LineString;
        }
        if ([value isEqualToString:@"Polygon"]) {
            return mbgl::FeatureType::Polygon;
        }
    } else if ([value isKindOfClass:NSNumber.class]) {
        switch ([value integerValue]) {
            case 1:
                return mbgl::FeatureType::Point;
            case 2:
                return mbgl::FeatureType::LineString;
            case 3:
                return mbgl::FeatureType::Polygon;
            default:
                break;
        }
    }
    return mbgl::FeatureType::Unknown;
}

- (std::vector<mbgl::FeatureIdentifier>)mgl_aggregateFeatureIdentifier {
    if ([self.constantValue isKindOfClass:[NSArray class]] || [self.constantValue isKindOfClass:[NSSet class]]) {
        std::vector<mbgl::FeatureIdentifier> convertedValues;
        for (id value in self.constantValue) {
            NSExpression *expression = value;
            if (![expression isKindOfClass:[NSExpression class]]) {
                expression = [NSExpression expressionForConstantValue:expression];
            }
            convertedValues.push_back(expression.mgl_featureIdentifier);
        }
        return convertedValues;
    }
    [NSException raise:NSInvalidArgumentException
                format:@"Constant value expression must contain an array or set."];
    return {};
}

- (mbgl::FeatureIdentifier)mgl_featureIdentifier {
    mbgl::Value mbglValue = self.mgl_constantMBGLValue;

    if (mbglValue.is<std::string>()) {
        return mbglValue.get<std::string>();
    }
    if (mbglValue.is<double>()) {
        return mbglValue.get<double>();
    }
    if (mbglValue.is<uint64_t>()) {
        return mbglValue.get<uint64_t>();
    }
    if (mbglValue.is<int64_t>()) {
        return mbglValue.get<int64_t>();
    }

    return {};
}

@end

@implementation NSObject (MLNExpressionAdditions)

- (NSNumber *)mgl_number {
    return nil;
}

- (NSNumber *)mgl_numberWithFallbackValues:(id)fallbackValue, ... {
    if (self.mgl_number) {
        return self.mgl_number;
    }

    va_list fallbackValues;
    va_start(fallbackValues, fallbackValue);
    for (id value = fallbackValue; value; value = va_arg(fallbackValues, id)) {
        if ([value mgl_number]) {
            return [value mgl_number];
        }
    }

    return nil;
}

@end

@implementation NSNull (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    return self;
}

@end

@implementation NSString (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    return self;
}

- (NSNumber *)mgl_number {
    if (self.doubleValue || ![[NSDecimalNumber decimalNumberWithString:self] isEqual:[NSDecimalNumber notANumber]]) {
        return @(self.doubleValue);
    }

    return nil;
}

@end

@implementation NSNumber (MLNExpressionAdditions)

- (id)mgl_interpolateWithCurveType:(NSString *)curveType
                        parameters:(NSArray *)parameters
                             stops:(NSDictionary<NSNumber *, id> *)stops {
    [NSException raise:NSInvalidArgumentException
                format:@"Interpolation expressions lack underlying Objective-C implementations."];
    return nil;
}

- (id)mgl_stepWithMinimum:(id)minimum stops:(NSDictionary<NSNumber *, id> *)stops {
    [NSException raise:NSInvalidArgumentException
                format:@"Interpolation expressions lack underlying Objective-C implementations."];
    return nil;
}

- (NSNumber *)mgl_number {
    return self;
}

- (id)mgl_jsonExpressionObject {
    if ([self isEqualToNumber:@(M_E)]) {
        return @[@"e"];
    } else if ([self isEqualToNumber:@(M_PI)]) {
        return @[@"pi"];
    }
    return self;
}

@end

@implementation MLNColor (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    auto color = [self mgl_colorForPremultipliedValue];
    if (color.a == 1) {
        return @[@"rgb", @(color.r * 255), @(color.g * 255), @(color.b * 255)];
    }
    return @[@"rgba", @(color.r * 255), @(color.g * 255), @(color.b * 255), @(color.a)];
}

@end

@implementation NSArray (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    return [self valueForKeyPath:@"mgl_jsonExpressionObject"];
}

- (id)mgl_coalesce {
    [NSException raise:NSInvalidArgumentException
                      format:@"Coalesce expressions lack underlying Objective-C implementations."];
    return nil;
}

@end

@implementation NSDictionary (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    NSMutableDictionary *expressionObject = [NSMutableDictionary dictionaryWithCapacity:self.count];
    [self enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL * _Nonnull stop) {
        expressionObject[[key mgl_jsonExpressionObject]] = [obj mgl_jsonExpressionObject];
    }];

    return expressionObject;
}

- (id)mgl_has:(id)element {
    [NSException raise:NSInvalidArgumentException
                format:@"Has expressions lack underlying Objective-C implementations."];
    return nil;

}

@end

@implementation NSExpression (MLNExpressionAdditions)

- (NSExpression *)mgl_expressionWithContext:(NSDictionary<NSString *, NSExpression *> *)context {
    [NSException raise:NSInternalInconsistencyException
                format:@"Assignment expressions lack underlying Objective-C implementations."];
    return self;
}

- (id)mgl_has:(id)element {
    [NSException raise:NSInvalidArgumentException
                format:@"Has expressions lack underlying Objective-C implementations."];
    return nil;
}

@end

@implementation MLNShape (MLNExpressionAdditions)

- (id)mgl_jsonExpressionObject {
    return self.geoJSONDictionary;
}

@end

@implementation NSExpression (MLNAdditions)

+ (NSExpression *)zoomLevelVariableExpression {
    return [NSExpression expressionForVariable:@"zoomLevel"];
}

+ (NSExpression *)heatmapDensityVariableExpression {
    return [NSExpression expressionForVariable:@"heatmapDensity"];
}

+ (NSExpression *)lineProgressVariableExpression {
    return [NSExpression expressionForVariable:@"lineProgress"];
}

+ (NSExpression *)featureAccumulatedVariableExpression {
    return [NSExpression expressionForVariable:@"featureAccumulated"];
}

+ (NSExpression *)geometryTypeVariableExpression {
    return [NSExpression expressionForVariable:@"geometryType"];
}

+ (NSExpression *)featureIdentifierVariableExpression {
    return [NSExpression expressionForVariable:@"featureIdentifier"];
}

+ (NSExpression *)featureAttributesVariableExpression {
    return [NSExpression expressionForVariable:@"featureAttributes"];
}

+ (NSExpression *)featurePropertiesVariableExpression {
    return [self featureAttributesVariableExpression];
}

+ (instancetype)mgl_expressionForConditional:(nonnull NSPredicate *)conditionPredicate trueExpression:(nonnull NSExpression *)trueExpression falseExpresssion:(nonnull NSExpression *)falseExpression {
    return [NSExpression expressionForConditional:conditionPredicate trueExpression:trueExpression falseExpression:falseExpression];
}

+ (instancetype)mgl_expressionForSteppingExpression:(nonnull NSExpression *)steppingExpression fromExpression:(nonnull NSExpression *)minimumExpression stops:(nonnull NSExpression *)stops {
    NSString *selectorName = @"mgl_step:from:stops:";

    if (@available(iOS 15.5, macOS 12, *)) {
        return [NSExpression expressionForFunction:steppingExpression
                                      selectorName:selectorName
                                         arguments:@[steppingExpression, minimumExpression, stops]];
    } else {
        return [NSExpression expressionForFunction:selectorName
                                         arguments:@[steppingExpression, minimumExpression, stops]];
    }
}

+ (instancetype)mgl_expressionForInterpolatingExpression:(nonnull NSExpression *)inputExpression withCurveType:(nonnull MLNExpressionInterpolationMode)curveType parameters:(nullable NSExpression *)parameters stops:(nonnull NSExpression *)stops {
    NSExpression *sanitizeParams = parameters ? parameters : [NSExpression expressionForConstantValue:nil];

    NSString *selectorName = @"mgl_interpolate:withCurveType:parameters:stops:";

    if (@available(iOS 15.5, macOS 12, *)) {
        return [NSExpression expressionForFunction:inputExpression
                                      selectorName:selectorName
                                         arguments:@[inputExpression, [NSExpression expressionForConstantValue:curveType], sanitizeParams, stops]];
    } else {
        return [NSExpression expressionForFunction:selectorName
                                         arguments:@[inputExpression, [NSExpression expressionForConstantValue:curveType], sanitizeParams, stops]];
    }
}

+ (instancetype)mgl_expressionForMatchingExpression:(nonnull NSExpression *)inputExpression inDictionary:(nonnull NSDictionary<NSExpression *, NSExpression *> *)matchedExpressions defaultExpression:(nonnull NSExpression *)defaultExpression {
    NSMutableArray *optionsArray = [NSMutableArray arrayWithObjects:inputExpression, nil];

    NSEnumerator *matchEnumerator = matchedExpressions.keyEnumerator;
    while (NSExpression *key = matchEnumerator.nextObject) {
        [optionsArray addObject:key];
        [optionsArray addObject:[matchedExpressions objectForKey:key]];
    }

    [optionsArray addObject:defaultExpression];
    return [NSExpression expressionForFunction:@"MLN_MATCH"
                                     arguments:optionsArray];
}

+ (instancetype)mgl_expressionForAttributedExpressions:(nonnull NSArray<NSExpression *> *)attributedExpressions {
    return [NSExpression expressionForFunction:@"mgl_attributed:" arguments:attributedExpressions];
}

- (instancetype)mgl_expressionByAppendingExpression:(nonnull NSExpression *)expression {
    NSExpression *subexpression = [NSExpression expressionForAggregate:@[self, expression]];
    return [NSExpression expressionForFunction:@"mgl_join:" arguments:@[subexpression]];
}

static NSDictionary<NSString *, NSString *> *MLNFunctionNamesByExpressionOperator;
static NSDictionary<NSString *, NSString *> *MLNExpressionOperatorsByFunctionNames;

NSArray *MLNSubexpressionsWithJSONObjects(NSArray *objects) {
    NSMutableArray *subexpressions = [NSMutableArray arrayWithCapacity:objects.count];
    for (id object in objects) {
        NSExpression *expression = [NSExpression expressionWithMLNJSONObject:object];
        [subexpressions addObject:expression];
    }
    return subexpressions;
}

+ (instancetype)expressionWithMLNJSONObject:(id)object {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        MLNFunctionNamesByExpressionOperator = @{
            @"+": @"add:to:",
            @"-": @"from:subtract:",
            @"*": @"multiply:by:",
            @"/": @"divide:by:",
            @"%": @"modulus:by:",
            @"sqrt": @"sqrt:",
            @"log10": @"log:",
            @"ln": @"ln:",
            @"abs": @"abs:",
            @"round": @"mgl_round:",
            @"acos" : @"mgl_acos:",
            @"cos" : @"mgl_cos:",
            @"asin" : @"mgl_asin:",
            @"sin" : @"mgl_sin:",
            @"atan" : @"mgl_atan:",
            @"tan" : @"mgl_tan:",
            @"log2" : @"mgl_log2:",
            @"floor": @"floor:",
            @"ceil": @"ceiling:",
            @"^": @"raise:toPower:",
            @"distance": @"mgl_distanceFrom:",
            @"upcase": @"uppercase:",
            @"downcase": @"lowercase:",
            @"let": @"MLN_LET",
        };
    });
    if (!object || object == [NSNull null]) {
        return [NSExpression expressionForConstantValue:nil];
    }

    if ([object isKindOfClass:[NSString class]] ||
        [object isKindOfClass:[NSNumber class]] ||
        [object isKindOfClass:[NSValue class]] ||
        [object isKindOfClass:[MLNColor class]] ||
        [object isKindOfClass:[MLNShape class]]) {
        return [NSExpression expressionForConstantValue:object];
    }

    if ([object isKindOfClass:[NSDictionary class]]) {
        if (object[@"type"]) {
            NSError *error;
            NSData *shapeData = [NSJSONSerialization dataWithJSONObject:object options:0 error:&error];
            MLNShape *shape;
            if (shapeData && !error) {
                shape = [MLNShape shapeWithData:shapeData encoding:NSUTF8StringEncoding error:&error];
            }
            if (shape && !error) {
                return [NSExpression expressionForConstantValue:shape];
            }
        }

        NSMutableDictionary *dictionary = [NSMutableDictionary dictionaryWithCapacity:[object count]];
        [object enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL * _Nonnull stop) {
            dictionary[key] = [NSExpression expressionWithMLNJSONObject:obj];
        }];
        return [NSExpression expressionForConstantValue:dictionary];
    }
    if ([object isKindOfClass:[NSArray class]]) {
        NSArray *array = (NSArray *)object;
        NSString *op = array.firstObject;

        if (![op isKindOfClass:[NSString class]]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(array);
            return [NSExpression expressionForFunction:@"MLN_FUNCTION" arguments:subexpressions];
        }

        NSArray *argumentObjects = [array subarrayWithRange:NSMakeRange(1, array.count - 1)];

        NSString *functionName = MLNFunctionNamesByExpressionOperator[op];
        if (functionName) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            if ([op isEqualToString:@"+"] && argumentObjects.count > 2) {
                NSExpression *subexpression = [NSExpression expressionForAggregate:subexpressions];
                return [NSExpression expressionForFunction:@"sum:"
                                                 arguments:@[subexpression]];
            } else if ([op isEqualToString:@"^"] && [argumentObjects.firstObject isEqual:@[@"e"]]) {
                functionName = @"exp:";
                subexpressions = [subexpressions subarrayWithRange:NSMakeRange(1, subexpressions.count - 1)];
            }

            return [NSExpression expressionForFunction:functionName
                                             arguments:subexpressions];
        } else if ([op isEqualToString:@"collator"]) {
            // Avoid wrapping collator options object in literal expression.
            return [NSExpression expressionForFunction:@"MLN_FUNCTION" arguments:array];
        } else if ([op isEqualToString:@"literal"]) {
            if ([argumentObjects.firstObject isKindOfClass:[NSArray class]]) {
                return [NSExpression expressionForAggregate:MLNSubexpressionsWithJSONObjects(argumentObjects.firstObject)];
            }
            return [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
        } else if ([op isEqualToString:@"to-boolean"]) {
            NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
            return [NSExpression expressionForFunction:operand selectorName:@"boolValue" arguments:@[]];
        } else if ([op isEqualToString:@"to-number"] || [op isEqualToString:@"number"]) {
            NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
            if (argumentObjects.count == 1) {
                return [NSExpression expressionWithFormat:@"CAST(%@, 'NSNumber')", operand];
            }
            argumentObjects = [argumentObjects subarrayWithRange:NSMakeRange(1, argumentObjects.count - 1)];
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            return [NSExpression expressionForFunction:operand selectorName:@"mgl_numberWithFallbackValues:" arguments:subexpressions];
        } else if ([op isEqualToString:@"to-string"] || [op isEqualToString:@"string"]) {
            NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
            return [NSExpression expressionWithFormat:@"CAST(%@, 'NSString')", operand];
        } else if ([op isEqualToString:@"to-color"]) {
            NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];

            if (argumentObjects.count == 1) {
#if TARGET_OS_IPHONE
                return [NSExpression expressionWithFormat:@"CAST(%@, 'UIColor')", operand];
#else
                return [NSExpression expressionWithFormat:@"CAST(%@, 'NSColor')", operand];
#endif
            }
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(array);
            return [NSExpression expressionForFunction:@"MLN_FUNCTION" arguments:subexpressions];

        } else if ([op isEqualToString:@"to-rgba"]) {
            NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
            return [NSExpression expressionWithFormat:@"CAST(noindex(%@), 'NSArray')", operand];
        } else if ([op isEqualToString:@"get"]) {
            if (argumentObjects.count == 2) {
                NSExpression *operand = [NSExpression expressionWithMLNJSONObject:argumentObjects.lastObject];
                if ([argumentObjects.firstObject isKindOfClass:[NSString class]]) {
                    return [NSExpression expressionWithFormat:@"%@.%K", operand, argumentObjects.firstObject];
                }
                NSExpression *key = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
                return [NSExpression expressionWithFormat:@"%@.%@", operand, key];
            }
            return [NSExpression expressionForKeyPath:argumentObjects.firstObject];
        } else if ([op isEqualToString:@"length"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSString *function = @"count:";
            if ([subexpressions.firstObject expressionType] == NSConstantValueExpressionType
                && [[subexpressions.firstObject constantValue] isKindOfClass:[NSString class]]) {
                function = @"length:";
            }
            return [NSExpression expressionForFunction:function arguments:@[subexpressions.firstObject]];
        } else if ([op isEqualToString:@"rgb"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            return [NSExpression mgl_expressionForRGBComponents:subexpressions];
        } else if ([op isEqualToString:@"rgba"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            return [NSExpression mgl_expressionForRGBAComponents:subexpressions];
        } else if ([op isEqualToString:@"min"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSExpression *subexpression = [NSExpression expressionForAggregate:subexpressions];
            return [NSExpression expressionForFunction:@"min:" arguments:@[subexpression]];
        } else if ([op isEqualToString:@"max"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSExpression *subexpression = [NSExpression expressionForAggregate:subexpressions];
            return [NSExpression expressionForFunction:@"max:" arguments:@[subexpression]];
        } else if ([op isEqualToString:@"e"]) {
            return [NSExpression expressionForConstantValue:@(M_E)];
        } else if ([op isEqualToString:@"pi"]) {
            return [NSExpression expressionForConstantValue:@(M_PI)];
        } else if ([op isEqualToString:@"concat"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSExpression *subexpression = [NSExpression expressionForAggregate:subexpressions];
            return [NSExpression expressionForFunction:@"mgl_join:" arguments:@[subexpression]];
        }  else if ([op isEqualToString:@"at"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSExpression *index = subexpressions.firstObject;
            NSExpression *operand = subexpressions[1];
            return [NSExpression expressionForFunction:@"objectFrom:withIndex:" arguments:@[operand, index]];
        } else if ([op isEqualToString:@"has"]) {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(argumentObjects);
            NSExpression *operand = argumentObjects.count > 1 ? subexpressions[1] : [NSExpression expressionForEvaluatedObject];
            NSExpression *key = subexpressions.firstObject;
            return [NSExpression expressionForFunction:@"mgl_does:have:" arguments:@[operand, key]];
        } else if ([op isEqualToString:@"interpolate"]) {
            NSArray *interpolationOptions = argumentObjects.firstObject;
            NSString *curveType = interpolationOptions.firstObject;
            MLNExpressionInterpolationMode interpolationMode = MLNExpressionInterpolationModeLinear;
            id curveParameters;
            if ([curveType isEqual:@"exponential"]) {
                curveParameters = interpolationOptions[1];
                interpolationMode = MLNExpressionInterpolationModeExponential;
            } else if ([curveType isEqualToString:@"cubic-bezier"]) {
                curveParameters = @[@"literal", [interpolationOptions subarrayWithRange:NSMakeRange(1, 4)]];
                interpolationMode = MLNExpressionInterpolationModeCubicBezier;
            }
            else {
                curveParameters = [NSNull null];
            }
            NSExpression *curveParameterExpression = [NSExpression expressionWithMLNJSONObject:curveParameters];
            argumentObjects = [argumentObjects subarrayWithRange:NSMakeRange(1, argumentObjects.count - 1)];
            NSExpression *inputExpression = [NSExpression expressionWithMLNJSONObject:argumentObjects.firstObject];
            NSArray *stopExpressions = [argumentObjects subarrayWithRange:NSMakeRange(1, argumentObjects.count - 1)];
            NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:stopExpressions.count / 2];
            NSEnumerator *stopEnumerator = stopExpressions.objectEnumerator;
            while (NSNumber *key = stopEnumerator.nextObject) {
                NSExpression *valueExpression = stopEnumerator.nextObject;
                stops[key] = [NSExpression expressionWithMLNJSONObject:valueExpression];
            }
            NSExpression *stopExpression = [NSExpression expressionForConstantValue:stops];
            return [NSExpression mgl_expressionForInterpolatingExpression:inputExpression
                                                            withCurveType:interpolationMode
                                                               parameters:curveParameterExpression
                                                                    stops:stopExpression];
        } else if ([op isEqualToString:@"step"]) {
            NSExpression *inputExpression = [NSExpression expressionWithMLNJSONObject:argumentObjects[0]];
            NSArray *stopExpressions = [argumentObjects subarrayWithRange:NSMakeRange(1, argumentObjects.count - 1)];
            NSExpression *minimum;
            if (stopExpressions.count % 2) {
                minimum = [NSExpression expressionWithMLNJSONObject:stopExpressions.firstObject];
                stopExpressions = [stopExpressions subarrayWithRange:NSMakeRange(1, stopExpressions.count - 1)];
            }
            NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:stopExpressions.count / 2];
            NSEnumerator *stopEnumerator = stopExpressions.objectEnumerator;
            while (NSNumber *key = stopEnumerator.nextObject) {
                NSExpression *valueExpression = stopEnumerator.nextObject;
                if (minimum) {
                    stops[key] = [NSExpression expressionWithMLNJSONObject:valueExpression];
                } else {
                    minimum = [NSExpression expressionWithMLNJSONObject:valueExpression];
                }
            }

            NSAssert(minimum, @"minimum should be non-nil");
            if (minimum) {
                NSExpression *stopExpression = [NSExpression expressionForConstantValue:stops];
                return [NSExpression mgl_expressionForSteppingExpression:inputExpression
                                                          fromExpression:minimum
                                                                   stops:stopExpression];
            }

        } else if ([op isEqualToString:@"zoom"]) {
            return NSExpression.zoomLevelVariableExpression;
        } else if ([op isEqualToString:@"heatmap-density"]) {
            return NSExpression.heatmapDensityVariableExpression;
        } else if ([op isEqualToString:@"line-progress"]) {
            return NSExpression.lineProgressVariableExpression;
        } else if ([op isEqualToString:@"accumulated"]) {
            return NSExpression.featureAccumulatedVariableExpression;
        } else if ([op isEqualToString:@"geometry-type"]) {
            return NSExpression.geometryTypeVariableExpression;
        } else if ([op isEqualToString:@"id"]) {
            return NSExpression.featureIdentifierVariableExpression;
        }  else if ([op isEqualToString:@"properties"]) {
            return NSExpression.featureAttributesVariableExpression;
        } else if ([op isEqualToString:@"var"]) {
            return [NSExpression expressionForVariable:argumentObjects.firstObject];
        } else if ([op isEqualToString:@"case"]) {
            NSMutableArray *arguments = [NSMutableArray array];

            for (NSUInteger index = 0; index < argumentObjects.count; index++) {
                if (index % 2 == 0 && index != argumentObjects.count - 1) {
                    NSPredicate *predicate = [NSPredicate predicateWithMLNJSONObject:argumentObjects[index]];
                    NSExpression *argument = [NSExpression expressionForConstantValue:predicate];
                    [arguments addObject:argument];
                } else {
                    [arguments addObject:[NSExpression expressionWithMLNJSONObject:argumentObjects[index]]];
                }
            }

            if (arguments.count == 3) {
                NSPredicate *conditional = [arguments.firstObject constantValue];
                return [NSExpression expressionForConditional:conditional trueExpression:arguments[1] falseExpression:arguments[2]];
            }
            return [NSExpression expressionForFunction:@"MLN_IF" arguments:arguments];
        } else if ([op isEqualToString:@"match"]) {
            NSMutableArray *optionsArray = [NSMutableArray array];

            for (NSUInteger index = 0; index < argumentObjects.count; index++) {
                NSExpression *option = [NSExpression expressionWithMLNJSONObject:argumentObjects[index]];
                // match operators with arrays as matching values should not parse arrays as generic functions.
                if (index > 0 && index < argumentObjects.count - 1 && !(index % 2 == 0) && [argumentObjects[index] isKindOfClass:[NSArray class]]) {
                    option = [NSExpression expressionForAggregate:MLNSubexpressionsWithJSONObjects(argumentObjects[index])];
                }
                [optionsArray addObject:option];
            }

            return [NSExpression expressionForFunction:@"MLN_MATCH"
                                             arguments:optionsArray];
        } else if ([op isEqualToString:@"format"]) {
            NSMutableArray *attributedExpressions = [NSMutableArray array];

            for (NSUInteger index = 0; index < argumentObjects.count; index+=2) {
                NSExpression *expression = [NSExpression expressionWithMLNJSONObject:argumentObjects[index]];
                NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
                if ((index + 1) < argumentObjects.count) {
                    attrs = [NSMutableDictionary dictionaryWithDictionary:argumentObjects[index + 1]];
                }

                for (NSString *key in attrs.allKeys) {
                    attrs[key] = [NSExpression expressionWithMLNJSONObject:attrs[key]];
                }
                MLNAttributedExpression *attributedExpression = [[MLNAttributedExpression alloc] initWithExpression:expression attributes:attrs];

                [attributedExpressions addObject:[NSExpression expressionForConstantValue:attributedExpression]];
            }
            return [NSExpression expressionForFunction:@"mgl_attributed:" arguments:attributedExpressions];

        } else if ([op isEqualToString:@"coalesce"]) {
            NSMutableArray *expressions = [NSMutableArray array];
            for (id operand in argumentObjects) {
                [expressions addObject:[NSExpression expressionWithMLNJSONObject:operand]];
            }

            return [NSExpression expressionWithFormat:@"mgl_coalesce(%@)", expressions];
        } else {
            NSArray *subexpressions = MLNSubexpressionsWithJSONObjects(array);
            return [NSExpression expressionForFunction:@"MLN_FUNCTION" arguments:subexpressions];
        }
    }

    [NSException raise:NSInvalidArgumentException
                format:@"Unable to convert JSON object %@ to an NSExpression.", object];

    return nil;
}

- (id)mgl_jsonExpressionObject {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        MLNExpressionOperatorsByFunctionNames = @{
            @"add:to:": @"+",
            @"from:subtract:": @"-",
            @"multiply:by:": @"*",
            @"divide:by:": @"/",
            @"modulus:by:": @"%",
            @"sqrt:": @"sqrt",
            @"log:": @"log10",
            @"ln:": @"ln",
            @"raise:toPower:": @"^",
            @"ceiling:": @"ceil",
            @"abs:": @"abs",
            @"floor:": @"floor",
            @"uppercase:": @"upcase",
            @"lowercase:": @"downcase",
            @"length:": @"length",
            @"mgl_round:": @"round",
            @"mgl_acos:" : @"acos",
            @"mgl_cos:" : @"cos",
            @"mgl_asin:" : @"asin",
            @"mgl_sin:" : @"sin",
            @"mgl_atan:" : @"atan",
            @"mgl_tan:" : @"tan",
            @"mgl_log2:" : @"log2",
            @"mgl_distanceFrom:": @"distance",
            // Vararg aftermarket expressions need to be declared with an explicit and implicit first argument.
            @"MLN_LET": @"let",
            @"MLN_LET:": @"let",
        };
    });

    switch (self.expressionType) {
        case NSVariableExpressionType: {
            if ([self.variable isEqualToString:@"heatmapDensity"]) {
                return @[@"heatmap-density"];
            }
            if ([self.variable isEqualToString:@"lineProgress"]) {
                return @[@"line-progress"];
            }
            if ([self.variable isEqualToString:@"zoomLevel"]) {
                return @[@"zoom"];
            }
            if ([self.variable isEqualToString:@"featureAccumulated"]) {
                return @[@"accumulated"];
            }
            if ([self.variable isEqualToString:@"geometryType"]) {
                return @[@"geometry-type"];
            }
            if ([self.variable isEqualToString:@"featureIdentifier"]) {
                return @[@"id"];
            }
            if ([self.variable isEqualToString:@"featureAttributes"]) {
                return @[@"properties"];
            }
            return @[@"var", self.variable];
        }

        case NSConstantValueExpressionType: {
            id constantValue = self.constantValue;
            if (!constantValue || constantValue == [NSNull null]) {
                return [NSNull null];
            }
            if ([constantValue isEqual:@(M_E)]) {
                return @[@"e"];
            }
            if ([constantValue isEqual:@(M_PI)]) {
                return @[@"pi"];
            }
            if ([constantValue isKindOfClass:[NSArray class]] ||
                [constantValue isKindOfClass:[NSDictionary class]]) {
                NSArray *collection = [constantValue mgl_jsonExpressionObject];
                return @[@"literal", collection];
            }
            if ([constantValue isKindOfClass:[MLNColor class]]) {
                auto color = [constantValue mgl_colorForPremultipliedValue];
                if (color.a == 1) {
                    return @[@"rgb", @(color.r * 255), @(color.g * 255), @(color.b * 255)];
                }
                return @[@"rgba", @(color.r * 255), @(color.g * 255), @(color.b * 255), @(color.a)];
            }
            if ([constantValue isKindOfClass:[NSValue class]]) {
                const auto boxedValue = (NSValue *)constantValue;
                if (strcmp([boxedValue objCType], @encode(CGVector)) == 0) {
                    // offset [x, y]
                    std::array<float, 2> mglValue = boxedValue.mgl_offsetArrayValue;
                    return @[@"literal", @[@(mglValue[0]), @(mglValue[1])]];
                }
                if (strcmp([boxedValue objCType], @encode(MLNEdgeInsets)) == 0) {
                    // padding [x, y]
                    std::array<float, 4> mglValue = boxedValue.mgl_paddingArrayValue;
                    return @[@"literal", @[@(mglValue[0]), @(mglValue[1]), @(mglValue[2]), @(mglValue[3])]];
                }
            }
            if ([constantValue isKindOfClass:[MLNAttributedExpression class]]) {
                MLNAttributedExpression *attributedExpression = (MLNAttributedExpression *)constantValue;
                id jsonObject = attributedExpression.expression.mgl_jsonExpressionObject;
                NSMutableDictionary<MLNAttributedExpressionKey, NSExpression *> *attributedDictionary = [NSMutableDictionary dictionary];

                if (attributedExpression.attributes) {
                    attributedDictionary = [NSMutableDictionary dictionaryWithDictionary:attributedExpression.attributes];

                    for (NSString *key in attributedExpression.attributes.allKeys) {
                        attributedDictionary[key] = attributedExpression.attributes[key].mgl_jsonExpressionObject;
                    }

                }
                return @[jsonObject, attributedDictionary];
            }
            if ([constantValue isKindOfClass:[MLNShape class]]) {
                MLNShape *shape = (MLNShape *)constantValue;
                return shape.geoJSONDictionary;
            }
            return self.constantValue;
        }

        case NSKeyPathExpressionType: {
            NSArray *expressionObject;
            NSArray *keyPath = [self.keyPath componentsSeparatedByString:@"."];
            for (NSString *pathComponent in keyPath) {
                if (expressionObject) {
                    expressionObject = @[@"get", pathComponent, expressionObject];
                } else {
                    expressionObject = @[@"get", pathComponent];
                }
            }

            NSAssert(expressionObject.count > 0, @"expressionObject should be non-empty");

            // Return a non-null value to quieten static analysis
            return expressionObject ?: @[];
        }

        case NSFunctionExpressionType: {
            NSString *function = self.function;

            BOOL hasCollectionProperty = !( ! [self.arguments.firstObject isKindOfClass: [NSExpression class]] || self.arguments.firstObject.expressionType != NSAggregateExpressionType || self.arguments.firstObject.expressionType == NSSubqueryExpressionType);
            NSString *op = MLNExpressionOperatorsByFunctionNames[function];
            if (op) {
                NSArray *arguments = self.arguments.mgl_jsonExpressionObject;
                return [@[op] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"valueForKey:"] || [function isEqualToString:@"valueForKeyPath:"]) {
                return @[@"get", self.arguments.firstObject.mgl_jsonExpressionObject, self.operand.mgl_jsonExpressionObject];
            } else if ([function isEqualToString:@"average:"]) {
                NSExpression *sum = [NSExpression expressionForFunction:@"sum:" arguments:self.arguments];
                NSExpression *count = [NSExpression expressionForFunction:@"count:" arguments:self.arguments];
                return [NSExpression expressionForFunction:@"divide:by:" arguments:@[sum, count]].mgl_jsonExpressionObject;
            } else if ([function isEqualToString:@"sum:"]) {
                NSArray *arguments;
                if (hasCollectionProperty) {
                    arguments = [self.arguments.firstObject.collection valueForKeyPath:@"mgl_jsonExpressionObject"];
                } else {
                    arguments = [self.arguments valueForKeyPath:@"mgl_jsonExpressionObject"];
                }
                return [@[@"+"] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"count:"]) {
                NSArray *arguments = self.arguments.firstObject.mgl_jsonExpressionObject;
                return @[@"length", arguments];
            } else if ([function isEqualToString:@"min:"]) {
                NSArray *arguments;
                if (!hasCollectionProperty) {
                    arguments = [self.arguments valueForKeyPath:@"mgl_jsonExpressionObject"];
                } else {
                    arguments = [self.arguments.firstObject.collection valueForKeyPath:@"mgl_jsonExpressionObject"];
                }
                return [@[@"min"] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"max:"]) {
                NSArray *arguments;
                if (!hasCollectionProperty) {
                    arguments = [self.arguments valueForKeyPath:@"mgl_jsonExpressionObject"];
                } else {
                    arguments = [self.arguments.firstObject.collection valueForKeyPath:@"mgl_jsonExpressionObject"];
                }
                return [@[@"max"] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"exp:"]) {
                return [NSExpression expressionForFunction:@"raise:toPower:" arguments:@[@(M_E), self.arguments.firstObject]].mgl_jsonExpressionObject;
            } else if ([function isEqualToString:@"trunc:"]) {
                return [NSExpression expressionWithFormat:@"%@ - modulus:by:(%@, 1)",
                        self.arguments.firstObject, self.arguments.firstObject].mgl_jsonExpressionObject;
            } else if ([function isEqualToString:@"mgl_join:"]) {
                NSArray *arguments;
                if (!hasCollectionProperty) {
                    arguments = [self.arguments valueForKeyPath:@"mgl_jsonExpressionObject"];
                } else {
                    arguments = [self.arguments.firstObject.collection valueForKeyPath:@"mgl_jsonExpressionObject"];
                }
                return [@[@"concat"] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"stringByAppendingString:"]) {
                NSArray *arguments = self.arguments.mgl_jsonExpressionObject;
                return [@[@"concat", self.operand.mgl_jsonExpressionObject] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"objectFrom:withIndex:"]) {
                id index = self.arguments[1].mgl_jsonExpressionObject;

                if ([self.arguments[1] expressionType] == NSConstantValueExpressionType
                    && [[self.arguments[1] constantValue] isKindOfClass:[NSString class]]) {
                    id value = self.arguments[1].constantValue;

                    if ([value isEqualToString:@"FIRST"]) {
                        index = [NSExpression expressionForConstantValue:@0].mgl_jsonExpressionObject;
                    } else if ([value isEqualToString:@"LAST"]) {
                        index = [NSExpression expressionWithFormat:@"count(%@) - 1", self.arguments[0]].mgl_jsonExpressionObject;
                    } else if ([value isEqualToString:@"SIZE"]) {
                        return [NSExpression expressionWithFormat:@"count(%@)", self.arguments[0]].mgl_jsonExpressionObject;
                    }
                }

                return @[@"at", index, self.arguments[0].mgl_jsonExpressionObject];
            } else if ([function isEqualToString:@"boolValue"]) {
                return @[@"to-boolean", self.operand.mgl_jsonExpressionObject];
            } else if ([function isEqualToString:@"mgl_number"] ||
                       [function isEqualToString:@"mgl_numberWithFallbackValues:"] ||
                       [function isEqualToString:@"decimalValue"] ||
                       [function isEqualToString:@"floatValue"] ||
                       [function isEqualToString:@"doubleValue"]) {
                NSArray *arguments = self.arguments.mgl_jsonExpressionObject;
                return [@[@"to-number", self.operand.mgl_jsonExpressionObject] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"stringValue"]) {
                return @[@"to-string", self.operand.mgl_jsonExpressionObject];
            } else if ([function isEqualToString:@"noindex:"]) {
                return self.arguments.firstObject.mgl_jsonExpressionObject;
            } else if ([function isEqualToString:@"mgl_does:have:"] ||
                       [function isEqualToString:@"mgl_has:"]) {
                return self.mgl_jsonHasExpressionObject;
            } else if ([function isEqualToString:@"mgl_interpolate:withCurveType:parameters:stops:"]
                       || [function isEqualToString:@"mgl_interpolateWithCurveType:parameters:stops:"]) {
                return self.mgl_jsonInterpolationExpressionObject;
            } else if ([function isEqualToString:@"mgl_step:from:stops:"]
                       || [function isEqualToString:@"mgl_stepWithMinimum:stops:"]) {
                return self.mgl_jsonStepExpressionObject;
            } else if ([function isEqualToString:@"mgl_expressionWithContext:"]) {
                id context = self.arguments.firstObject;
                if ([context isKindOfClass:[NSExpression class]]) {
                    context = [context constantValue];
                }
                NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"let", nil];
                [context enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, NSExpression * _Nonnull obj, BOOL * _Nonnull stop) {
                    [expressionObject addObject:key];
                    [expressionObject addObject:obj.mgl_jsonExpressionObject];
                }];
                [expressionObject addObject:self.operand.mgl_jsonExpressionObject];
                return expressionObject;
            } else if ([function isEqualToString:@"MLN_IF"] ||
                       [function isEqualToString:@"MLN_IF:"] ||
                       [function isEqualToString:@"mgl_if:"]) {
                return self.mgl_jsonIfExpressionObject;
            } else if ([function isEqualToString:@"MLN_MATCH"] ||
                       [function isEqualToString:@"MLN_MATCH:"] ||
                       [function isEqualToString:@"mgl_match:"]) {
                return self.mgl_jsonMatchExpressionObject;
            } else if ([function isEqualToString:@"mgl_coalesce:"] ||
                       [function isEqualToString:@"mgl_coalesce"]) {

                return self.mgl_jsonCoalesceExpressionObject;
            } else if ([function isEqualToString:@"castObject:toType:"]) {
                id object = self.arguments.firstObject.mgl_jsonExpressionObject;
                NSString *type = self.arguments[1].mgl_jsonExpressionObject;
                if ([type isEqualToString:@"NSString"]) {
                    return @[@"to-string", object];
                } else if ([type isEqualToString:@"NSNumber"]) {
                    return @[@"to-number", object];
                }
#if TARGET_OS_IPHONE
                else if ([type isEqualToString:@"UIColor"] || [type isEqualToString:@"MLNColor"]) {
                    return @[@"to-color", object];
                }
#else
                else if ([type isEqualToString:@"NSColor"] || [type isEqualToString:@"MLNColor"]) {
                    return @[@"to-color", object];
                }
#endif
                else if ([type isEqualToString:@"NSArray"]) {
                    NSExpression *operand = self.arguments.firstObject;
                    if ([operand expressionType] == NSFunctionExpressionType ) {
                        operand = self.arguments.firstObject.arguments.firstObject;
                    }
                    if (([operand expressionType] != NSConstantValueExpressionType) ||
                        ([operand expressionType] == NSConstantValueExpressionType &&
                         [[operand constantValue] isKindOfClass:[MLNColor class]])) {
                        return @[@"to-rgba", object];
                    }
                }
                [NSException raise:NSInvalidArgumentException
                            format:@"Casting expression to %@ not yet implemented.", type];
            } else if ([function isEqualToString:@"mgl_attributed:"]) {
                return [self mgl_jsonFormatExpressionObject];

            } else if ([function isEqualToString:@"MLN_FUNCTION"] ||
                       [function isEqualToString:@"MLN_FUNCTION:"]) {
                NSExpression *firstOp = self.arguments.firstObject;
                if (firstOp.expressionType == NSConstantValueExpressionType
                    && [firstOp.constantValue isEqualToString:@"collator"]) {
                    // Avoid wrapping collator options object in literal expression.
                    return @[@"collator", self.arguments[1].constantValue];
                }
                if (firstOp.expressionType == NSConstantValueExpressionType
                    && [firstOp.constantValue isEqualToString:@"format"]) {
                    // Avoid wrapping format options object in literal expression.
                    NSMutableArray *expressionObject = [NSMutableArray array];
                    [expressionObject addObject:@"format"];

                    for (NSUInteger index = 1; index < self.arguments.count; index++) {
                        if (index % 2 == 1) {
                            [expressionObject addObject:self.arguments[index].mgl_jsonExpressionObject];
                        } else {
                            [expressionObject addObject:self.arguments[index].constantValue];
                        }

                    }

                    return expressionObject;
                }

                return self.arguments.mgl_jsonExpressionObject;
            } else if (op == (NSString *) [MLNColor class] && [function isEqualToString:@"colorWithRed:green:blue:alpha:"]) {
                NSArray *arguments = self.arguments.mgl_jsonExpressionObject;
                return [@[@"rgba"] arrayByAddingObjectsFromArray:arguments];
            } else if ([function isEqualToString:@"median:"] ||
                       [function isEqualToString:@"mode:"] ||
                       [function isEqualToString:@"stddev:"] ||
                       [function isEqualToString:@"random"] ||
                       [function isEqualToString:@"randomn:"] ||
                       [function isEqualToString:@"now"] ||
                       [function isEqualToString:@"bitwiseAnd:with:"] ||
                       [function isEqualToString:@"bitwiseOr:with:"] ||
                       [function isEqualToString:@"bitwiseXor:with:"] ||
                       [function isEqualToString:@"leftshift:by:"] ||
                       [function isEqualToString:@"rightshift:by:"] ||
                       [function isEqualToString:@"onesComplement:"] ||
                       [function isEqualToString:@"distanceToLocation:fromLocation:"]) {
                [NSException raise:NSInvalidArgumentException
                            format:@"Expression function %@ not yet implemented.", function];
                return nil;
            } else {
                [NSException raise:NSInvalidArgumentException
                            format:@"Unrecognized expression function %@.", function];
                return nil;
            }
        }

        case NSConditionalExpressionType: {
            NSMutableArray *arguments = [NSMutableArray arrayWithObjects:@"case", self.predicate.mgl_jsonExpressionObject, nil];
            [arguments addObject:self.trueExpression.mgl_jsonExpressionObject];
            [arguments addObject:self.falseExpression.mgl_jsonExpressionObject];

            return arguments;
        }

        case NSAggregateExpressionType: {
            NSArray *collection = [self.collection valueForKeyPath:@"mgl_jsonExpressionObject"];
            return @[@"literal", collection];
        }

        case NSEvaluatedObjectExpressionType:
        case NSUnionSetExpressionType:
        case NSIntersectSetExpressionType:
        case NSMinusSetExpressionType:
        case NSSubqueryExpressionType:
        case NSAnyKeyExpressionType:
        case NSBlockExpressionType:
            [NSException raise:NSInvalidArgumentException
                        format:@"Expression type %lu not yet implemented.", (unsigned long)self.expressionType];
    }

    // NSKeyPathSpecifierExpression
    if (self.expressionType == 10) {
        return self.description;
    }
    // An assignment expression type is present in the BNF grammar, but the
    // corresponding NSExpressionType value and property getters are missing.
    if (self.expressionType == 12) {
        [NSException raise:NSInvalidArgumentException
                    format:@"Assignment expressions not yet implemented."];
    }

    return nil;
}

- (id)mgl_jsonInterpolationExpressionObject {
    NSUInteger expectedArgumentCount = [self.function componentsSeparatedByString:@":"].count - 1;
    if (self.arguments.count < expectedArgumentCount) {
        [NSException raise:NSInvalidArgumentException format:
         @"Too few arguments to ‘%@’ function; expected %lu arguments.",
         self.function, (unsigned long)expectedArgumentCount];
    } else if (self.arguments.count > expectedArgumentCount) {
        [NSException raise:NSInvalidArgumentException format:
         @"%lu unexpected arguments to ‘%@’ function; expected %lu arguments.",
         self.arguments.count - (unsigned long)expectedArgumentCount, self.function, (unsigned long)expectedArgumentCount];
    }

    BOOL isAftermarketFunction = [self.function isEqualToString:@"mgl_interpolate:withCurveType:parameters:stops:"];
    NSUInteger curveTypeIndex = isAftermarketFunction ? 1 : 0;
    NSString *curveType = self.arguments[curveTypeIndex].constantValue;
    NSMutableArray *interpolationArray = [NSMutableArray arrayWithObject:curveType];
    if ([curveType isEqualToString:@"exponential"]) {
        id base = [self.arguments[curveTypeIndex + 1] mgl_jsonExpressionObject];
        [interpolationArray addObject:base];
    } else if ([curveType isEqualToString:@"cubic-bezier"]) {
        NSArray *controlPoints = [self.arguments[curveTypeIndex + 1].collection mgl_jsonExpressionObject];
        [interpolationArray addObjectsFromArray:controlPoints];
    }

    NSDictionary<NSNumber *, NSExpression *> *stops = self.arguments[curveTypeIndex + 2].constantValue;

    if (stops.count == 0) {
        [NSException raise:NSInvalidArgumentException format:@"‘stops’ dictionary argument to ‘%@’ function must not be empty.", self.function];
    }

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"interpolate", interpolationArray, nil];
    [expressionObject addObject:(isAftermarketFunction ? self.arguments.firstObject : self.operand).mgl_jsonExpressionObject];
    for (NSNumber *key in [stops.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
        [expressionObject addObject:key];
        [expressionObject addObject:[stops[key] mgl_jsonExpressionObject]];
    }
    return expressionObject;
}

- (id)mgl_jsonStepExpressionObject {
    BOOL isAftermarketFunction = [self.function isEqualToString:@"mgl_step:from:stops:"];
    NSUInteger minimumIndex = isAftermarketFunction ? 1 : 0;
    id minimum = self.arguments[minimumIndex].mgl_jsonExpressionObject;
    NSDictionary<NSNumber *, NSExpression *> *stops = self.arguments[minimumIndex + 1].constantValue;

    if (stops.count == 0) {
        [NSException raise:NSInvalidArgumentException format:@"‘stops’ dictionary argument to ‘%@’ function must not be empty.", self.function];
    }

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"step", (isAftermarketFunction ? self.arguments.firstObject : self.operand).mgl_jsonExpressionObject, minimum, nil];

    for (NSNumber *key in [stops.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
        [expressionObject addObject:key];
        [expressionObject addObject:[stops[key] mgl_jsonExpressionObject]];
    }
    return expressionObject;
}

- (id)mgl_jsonMatchExpressionObject {
    BOOL isAftermarketFunction = [self.function hasPrefix:@"MLN_MATCH"];
    NSUInteger minimumIndex = isAftermarketFunction ? 1 : 0;

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"match", (isAftermarketFunction ? self.arguments.firstObject : self.operand).mgl_jsonExpressionObject, nil];
    NSArray<NSExpression *> *arguments = isAftermarketFunction ? self.arguments : self.arguments[minimumIndex].constantValue;

    for (NSUInteger index = minimumIndex; index < arguments.count; index++) {
        NSArray *argumentObject = arguments[index].mgl_jsonExpressionObject;
        // match operators with arrays as matching values should not parse arrays using the literal operator.
        if (index > 0 && index < arguments.count - 1 && !(index % 2 == 0)) {
            NSExpression *expression = arguments[index];
            if (![expression isKindOfClass:[NSExpression class]]) {
                expression = [NSExpression expressionForConstantValue:expression];
            }
            if (expression.expressionType == NSAggregateExpressionType ||
                (expression.expressionType == NSConstantValueExpressionType && [expression.constantValue isKindOfClass:[NSArray class]])) {
                argumentObject = argumentObject.count == 2 ? argumentObject[1] : argumentObject;
            }
        }
        [expressionObject addObject:argumentObject];
    }

    return expressionObject;
}

- (id)mgl_jsonIfExpressionObject {
    BOOL isAftermarketFunction = [self.function hasPrefix:@"MLN_IF"];
    NSUInteger minimumIndex = isAftermarketFunction ? 1 : 0;
    NSExpression *firstCondition;
    id condition;

    if (isAftermarketFunction) {
        firstCondition = self.arguments.firstObject;
    } else {
        firstCondition = self.operand;
    }

    if ([firstCondition respondsToSelector:@selector(constantValue)] && [firstCondition.constantValue isKindOfClass:[NSComparisonPredicate class]]) {
        NSPredicate *predicate = (NSPredicate *)firstCondition.constantValue;
        condition = predicate.mgl_jsonExpressionObject;
    } else {
        condition = firstCondition.mgl_jsonExpressionObject;
    }

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"case", condition, nil];
    NSArray<NSExpression *> *arguments = isAftermarketFunction ? self.arguments : self.arguments[minimumIndex].constantValue;

    for (NSUInteger index = minimumIndex; index < arguments.count; index++) {
        if ([arguments[index] respondsToSelector:@selector(constantValue)] && [arguments[index].constantValue isKindOfClass:[NSComparisonPredicate class]]) {
            NSPredicate *predicate = (NSPredicate *)arguments[index].constantValue;
            [expressionObject addObject:predicate.mgl_jsonExpressionObject];
        } else {
            [expressionObject addObject:arguments[index].mgl_jsonExpressionObject];
        }
    }

    return expressionObject;
}

- (id)mgl_jsonCoalesceExpressionObject {
    BOOL isAftermarketFunction = [self.function isEqualToString:@"mgl_coalesce:"];
    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"coalesce", nil];

    for (NSExpression *expression in  (isAftermarketFunction ? self.arguments.firstObject : self.operand).constantValue) {
        [expressionObject addObject:[expression mgl_jsonExpressionObject]];
    }

    return expressionObject;
}

- (id)mgl_jsonHasExpressionObject {
    BOOL isAftermarketFunction = [self.function isEqualToString:@"mgl_does:have:"];
    NSExpression *operand = isAftermarketFunction ? self.arguments[0] : self.operand;
    NSExpression *key = self.arguments[isAftermarketFunction ? 1 : 0];

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"has", key.mgl_jsonExpressionObject, nil];
    if (operand.expressionType != NSEvaluatedObjectExpressionType) {
        [expressionObject addObject:operand.mgl_jsonExpressionObject];
    }
    return expressionObject;
}

- (id)mgl_jsonFormatExpressionObject {
    NSArray<NSExpression *> *attributedExpressions;
    NSExpression *formatArray = self.arguments.firstObject;

    if ([formatArray respondsToSelector:@selector(constantValue)] && [formatArray.constantValue isKindOfClass:[NSArray class]]) {
        attributedExpressions = (NSArray *)formatArray.constantValue;
    } else {
        attributedExpressions = self.arguments;
    }

    NSMutableArray *expressionObject = [NSMutableArray arrayWithObjects:@"format", nil];

    for (NSUInteger index = 0; index < attributedExpressions.count; index++) {
        [expressionObject addObjectsFromArray:attributedExpressions[index].mgl_jsonExpressionObject];
    }

    return expressionObject;
}

// MARK: Localization

/**
 Returns a localized copy of the given collection.

 If no localization takes place, this method returns the original collection.
 */
NSArray<NSExpression *> *MLNLocalizedCollection(NSArray<NSExpression *> *collection, NSLocale * _Nullable locale) {
    __block NSMutableArray *localizedCollection;
    [collection enumerateObjectsUsingBlock:^(NSExpression * _Nonnull item, NSUInteger idx, BOOL * _Nonnull stop) {
        NSExpression *localizedItem = [item mgl_expressionLocalizedIntoLocale:locale];
        if (localizedItem != item) {
            if (!localizedCollection) {
                localizedCollection = [collection mutableCopy];
            }
            localizedCollection[idx] = localizedItem;
        }
    }];
    return localizedCollection ?: collection;
};

/**
 Returns a localized copy of the given stop dictionary.

 If no localization takes place, this method returns the original stop
 dictionary.
 */
NSDictionary<NSNumber *, NSExpression *> *MLNLocalizedStopDictionary(NSDictionary<NSNumber *, NSExpression *> *stops, NSLocale * _Nullable locale) {
    __block NSMutableDictionary *localizedStops;
    [stops enumerateKeysAndObjectsUsingBlock:^(id _Nonnull zoomLevel, NSExpression * _Nonnull value, BOOL * _Nonnull stop) {
        if (![value isKindOfClass:[NSExpression class]]) {
            value = [NSExpression expressionForConstantValue:value];
        }
        NSExpression *localizedValue = [value mgl_expressionLocalizedIntoLocale:locale];
        if (localizedValue != value) {
            if (!localizedStops) {
                localizedStops = [stops mutableCopy];
            }
            localizedStops[zoomLevel] = localizedValue;
        }
    }];
    return localizedStops ?: stops;
};

- (NSExpression *)mgl_expressionLocalizedIntoLocale:(nullable NSLocale *)locale {
    switch (self.expressionType) {
        case NSConstantValueExpressionType: {
            if ([self.constantValue isKindOfClass:[NSDictionary class]]) {
                NSDictionary *localizedStops = MLNLocalizedStopDictionary(self.constantValue, locale);
                if (localizedStops != self.constantValue) {
                    return [NSExpression expressionForConstantValue:localizedStops];
                }
            } else if ([self.constantValue isKindOfClass:[NSArray class]]) {
                NSArray *localizedValues = MLNLocalizedCollection(self.constantValue, locale);
                if (localizedValues != self.constantValue) {
                    return [NSExpression expressionForConstantValue:localizedValues];
                }
            } else if ([self.constantValue isKindOfClass:[MLNAttributedExpression class]]) {
                MLNAttributedExpression *attributedExpression = (MLNAttributedExpression *)self.constantValue;
                NSExpression *localizedExpression = [attributedExpression.expression mgl_expressionLocalizedIntoLocale:locale];
                MLNAttributedExpression *localizedAttributedExpression = [MLNAttributedExpression attributedExpression:localizedExpression attributes:attributedExpression.attributes];

                return [NSExpression expressionForConstantValue:localizedAttributedExpression];
            }
            return self;
        }

        case NSKeyPathExpressionType: {
            if ([self.keyPath isEqualToString:@"name"] || [self.keyPath hasPrefix:@"name_"]) {
                NSString *localizedKeyPath = @"name";
                if (![locale.localeIdentifier isEqualToString:@"mul"]) {
                    NSArray *preferences = locale ? @[locale.localeIdentifier] : [NSLocale preferredLanguages];
                    NSString *preferredLanguage = [MLNVectorTileSource preferredMapboxStreetsLanguageForPreferences:preferences];
                    if (preferredLanguage) {
                        localizedKeyPath = [NSString stringWithFormat:@"name_%@", preferredLanguage];
                    }
                }
                // If the keypath is `name`, no need to fallback
                if ([localizedKeyPath isEqualToString:@"name"]) {
                    return [NSExpression expressionForKeyPath:localizedKeyPath];
                }
                // If the keypath is `name_zh-Hans`, fallback to `name_zh` to `name`.
                // CN tiles might using `name_zh-CN` for Simplified Chinese.
                if ([localizedKeyPath isEqualToString:@"name_zh-Hans"]) {
                    return [NSExpression expressionWithFormat:@"mgl_coalesce({%K, %K, %K, %K})",
                            localizedKeyPath, @"name_zh-CN", @"name_zh", @"name"];
                }
                // Mapbox Streets v8 has `name_zh-Hant`, we should fallback to Simplified Chinese if the field has no value.
                if ([localizedKeyPath isEqualToString:@"name_zh-Hant"]) {
                    return [NSExpression expressionWithFormat:@"mgl_coalesce({%K, %K, %K, %K, %K})",
                            localizedKeyPath, @"name_zh-Hans", @"name_zh-CN", @"name_zh", @"name"];
                }

                // Other keypath fallback to `name`
                return [NSExpression expressionWithFormat:@"mgl_coalesce({%K, %K})", localizedKeyPath, @"name"];
            }
            return self;
        }

        case NSFunctionExpressionType: {
            NSExpression *operand = self.operand;
            NSExpression *localizedOperand = [operand mgl_expressionLocalizedIntoLocale:locale];

            NSArray *arguments = self.arguments;
            NSArray *localizedArguments = MLNLocalizedCollection(arguments, locale);
            if (localizedArguments != arguments) {
                return [NSExpression expressionForFunction:localizedOperand
                                              selectorName:self.function
                                                 arguments:localizedArguments];
            }
            if (localizedOperand != operand) {
                return [NSExpression expressionForFunction:localizedOperand
                                              selectorName:self.function
                                                 arguments:self.arguments];
            }
            return self;
        }

        case NSConditionalExpressionType: {
            NSExpression *trueExpression = self.trueExpression;
            NSExpression *localizedTrueExpression = [trueExpression mgl_expressionLocalizedIntoLocale:locale];
            NSExpression *falseExpression = self.falseExpression;
            NSExpression *localizedFalseExpression = [falseExpression mgl_expressionLocalizedIntoLocale:locale];
            if (localizedTrueExpression != trueExpression || localizedFalseExpression != falseExpression) {
                return [NSExpression expressionForConditional:self.predicate
                                               trueExpression:localizedTrueExpression
                                              falseExpression:localizedFalseExpression];
            }
            return self;
        }

        case NSAggregateExpressionType: {
            NSArray *collection = self.collection;
            if ([collection isKindOfClass:[NSArray class]]) {
                NSArray *localizedCollection = MLNLocalizedCollection(collection, locale);
                if (localizedCollection != collection) {
                    return [NSExpression expressionForAggregate:localizedCollection];
                }
            }
            return self;
        }

        default:
            return self;
    }
}

@end
