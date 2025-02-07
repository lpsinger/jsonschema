#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <regex>   // std::regex
#include <set>     // std::set
#include <utility> // std::move

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft4_core_ref(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  // Determine the label
  const auto type{ReferenceType::Static};
  const auto current{keyword_location(context)};
  assert(context.frame.contains({type, current}));
  const auto &entry{context.frame.at({type, current})};
  assert(context.references.contains({type, entry.pointer}));
  const auto &reference{context.references.at({type, entry.pointer})};
  const auto label{std::hash<std::string>{}(reference.destination)};

  // The label is already registered, so just jump to it
  if (context.labels.contains(label)) {
    return {make<SchemaCompilerControlJump>(context, label, {})};
  }

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  return {make<SchemaCompilerControlLabel>(context, label,
                                           compile(applicate(context, label),
                                                   empty_pointer, empty_pointer,
                                                   reference.destination))};
}

auto compiler_draft4_validation_type(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  if (context.value.is_string()) {
    const auto &type{context.value.to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Null, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Object, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer},
          {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::String, {}, SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (context.value.is_array() && context.value.size() == 1 &&
             context.value.front().is_string()) {
    const auto &type{context.value.front().to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Null, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Object, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer},
          {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::String, {}, SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (context.value.is_array()) {
    std::set<JSON::Type> types;
    for (const auto &type : context.value.as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.emplace(JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.emplace(JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.emplace(JSON::Type::Object);
      } else if (type_string == "array") {
        types.emplace(JSON::Type::Array);
      } else if (type_string == "number") {
        types.emplace(JSON::Type::Integer);
        types.emplace(JSON::Type::Real);
      } else if (type_string == "integer") {
        types.emplace(JSON::Type::Integer);
      } else if (type_string == "string") {
        types.emplace(JSON::Type::String);
      }
    }

    assert(types.size() >= context.value.size());
    return {make<SchemaCompilerAssertionTypeStrictAny>(
        context, std::move(types), {}, SchemaCompilerTargetType::Instance)};
  }

  return {};
}

auto compiler_draft4_validation_required(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());

  if (context.value.empty()) {
    return {};
  } else if (context.value.size() > 1) {
    std::set<JSON::String> properties;
    for (const auto &property : context.value.as_array()) {
      assert(property.is_string());
      properties.emplace(property.to_string());
    }

    return {make<SchemaCompilerAssertionDefinesAll>(
        context, std::move(properties),
        type_condition(context, JSON::Type::Object),
        SchemaCompilerTargetType::Instance)};
  } else {
    assert(context.value.front().is_string());
    return {make<SchemaCompilerAssertionDefines>(
        context, context.value.front().to_string(),
        type_condition(context, JSON::Type::Object),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_applicator_allof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  SchemaCompilerTemplate children;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    for (auto &&step : compile(applicate(context),
                               {static_cast<Pointer::Token::Index>(index)})) {
      children.push_back(std::move(step));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(context, SchemaCompilerValueNone{},
                                         std::move(children),
                                         SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_anyof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    disjunctors.push_back(make<SchemaCompilerInternalContainer>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalOr>(context, SchemaCompilerValueNone{},
                                        std::move(disjunctors),
                                        SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_oneof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    disjunctors.push_back(make<SchemaCompilerInternalContainer>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalXor>(context, SchemaCompilerValueNone{},
                                         std::move(disjunctors),
                                         SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_properties(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  if (context.value.empty()) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children;
  for (auto &[key, subschema] : context.value.as_object()) {
    auto substeps{compile(subcontext, {key}, {key})};
    // TODO: As an optimization, only emit an annotation if
    // `additionalProperties` is also declared in the same subschema Annotations
    // as such don't exist in Draft 4, so emit a private annotation instead
    substeps.push_back(make<SchemaCompilerAnnotationPrivate>(
        subcontext, JSON{key}, {}, SchemaCompilerTargetType::Instance));
    children.push_back(make<SchemaCompilerInternalContainer>(
        subcontext, SchemaCompilerValueNone{}, std::move(substeps),
        // TODO: As an optimization, avoid this condition if the subschema
        // declares `required` and includes the given key
        {make<SchemaCompilerAssertionDefines>(
            subcontext, key, {}, SchemaCompilerTargetType::Instance)}));
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),
      type_condition(context, JSON::Type::Object))};
}

auto compiler_draft4_applicator_patternproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  if (context.value.empty()) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children;

  // For each regular expression and corresponding subschema in the object
  for (auto &entry : context.value.as_object()) {
    auto substeps{compile(subcontext, {entry.first}, {})};

    // TODO: As an optimization, only emit an annotation if
    // `additionalProperties` is also declared in the same subschema Annotations
    // as such don't exist in Draft 4, so emit a private annotation instead The
    // evaluator will make sure the same annotation is not reported twice. For
    // example, if the same property matches more than one subschema in
    // `patternProperties`
    substeps.push_back(make<SchemaCompilerAnnotationPrivate>(
        subcontext,
        SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                             empty_pointer},
        {}, SchemaCompilerTargetType::InstanceParent));

    // The instance property matches the schema property regex
    SchemaCompilerTemplate loop_condition{make<SchemaCompilerAssertionRegex>(
        subcontext,
        SchemaCompilerValueRegex{
            std::regex{entry.first, std::regex::ECMAScript}, entry.first},
        {}, SchemaCompilerTargetType::InstanceBasename)};

    // Loop over the instance properties
    children.push_back(make<SchemaCompilerLoopProperties>(
        subcontext,
        // Treat this as an internal step
        false,
        {make<SchemaCompilerInternalContainer>(
            subcontext, SchemaCompilerValueNone{}, std::move(substeps),
            std::move(loop_condition))},
        SchemaCompilerTemplate{}));
  }

  // If the instance is an object...
  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionTypeStrict>(
          subcontext, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_additionalproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};

  // Evaluate the subschema against the current property if it
  // was NOT collected as an annotation on either "properties" or
  // "patternProperties"
  SchemaCompilerTemplate conjunctions{

      // TODO: As an optimization, avoid this condition if the subschema does
      // not declare `properties`
      make<SchemaCompilerInternalNoAnnotation>(
          subcontext,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"properties"}),

      // TODO: As an optimization, avoid this condition if the subschema does
      // not declare `patternProperties`
      make<SchemaCompilerInternalNoAnnotation>(
          subcontext,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"patternProperties"}),
  };

  SchemaCompilerTemplate wrapper{make<SchemaCompilerInternalContainer>(
      subcontext, SchemaCompilerValueNone{},
      compile(subcontext, empty_pointer, empty_pointer),
      {make<SchemaCompilerLogicalAnd>(subcontext, SchemaCompilerValueNone{},
                                      std::move(conjunctions),
                                      SchemaCompilerTemplate{})})};

  return {make<SchemaCompilerLoopProperties>(
      context, true, {std::move(wrapper)},

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionTypeStrict>(
          subcontext, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_validation_pattern(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_string());
  const auto &regex_string{context.value.to_string()};
  return {make<SchemaCompilerAssertionRegex>(
      context,
      SchemaCompilerValueRegex{std::regex{regex_string, std::regex::ECMAScript},
                               regex_string},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_format(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  if (!context.value.is_string()) {
    return {};
  }

  // Regular expressions

  static const std::string FORMAT_REGEX_IPV4{
      "^(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-"
      "9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-"
      "9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$"};

  const auto &format{context.value.to_string()};

  if (format == "uri") {
    return {make<SchemaCompilerAssertionStringType>(
        context, SchemaCompilerValueStringType::URI,
        type_condition(context, JSON::Type::String),
        SchemaCompilerTargetType::Instance)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<SchemaCompilerAssertionRegex>(                                \
        context,                                                               \
        SchemaCompilerValueRegex{                                              \
            std::regex{(regular_expression), std::regex::ECMAScript},          \
            (regular_expression)},                                             \
        type_condition(context, JSON::Type::String),                           \
        SchemaCompilerTargetType::Instance)};                                  \
  }

  COMPILE_FORMAT_REGEX("ipv4", FORMAT_REGEX_IPV4)

#undef COMPILE_FORMAT_REGEX

  return {};
}

auto compiler_draft4_applicator_not(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLogicalNot>(
      context, SchemaCompilerValueNone{},
      compile(applicate(context), empty_pointer, empty_pointer),
      SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_items(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};
  if (is_schema(context.value)) {
    return {make<SchemaCompilerLoopItems>(
        context, SchemaCompilerValueUnsignedInteger{0},
        compile(subcontext, empty_pointer, empty_pointer),

        // TODO: As an optimization, avoid this condition if the subschema
        // declares `type` to `array` already
        {make<SchemaCompilerAssertionTypeStrict>(
            subcontext, JSON::Type::Array, {},
            SchemaCompilerTargetType::Instance)})};
  }

  assert(context.value.is_array());
  const auto &array{context.value.as_array()};

  SchemaCompilerTemplate children;
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    const auto index{
        static_cast<std::size_t>(std::distance(array.cbegin(), iterator))};
    children.push_back(make<SchemaCompilerInternalContainer>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {index}, {index}),

        // TODO: As an optimization, avoid this condition if the subschema
        // declares a corresponding `minItems`
        {make<SchemaCompilerAssertionSizeGreater>(
            subcontext, index, {}, SchemaCompilerTargetType::Instance)}));
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionTypeStrict>(
          subcontext, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_additionalitems(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.schema.is_object());

  // Nothing to do here
  if (!context.schema.defines("items") ||
      context.schema.at("items").is_object()) {
    return {};
  }

  const auto cursor{
      (context.schema.defines("items") && context.schema.at("items").is_array())
          ? context.schema.at("items").size()
          : 0};

  return {make<SchemaCompilerLoopItems>(
      context, SchemaCompilerValueUnsignedInteger{cursor},
      compile(applicate(context), empty_pointer, empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_dependencies(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  SchemaCompilerTemplate children;
  const auto subcontext{applicate(context)};

  for (const auto &entry : context.value.as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make<SchemaCompilerInternalContainer>(
            subcontext, SchemaCompilerValueNone{},
            compile(subcontext, {entry.first}, empty_pointer),

            // TODO: As an optimization, avoid this condition if the subschema
            // declares `required` and includes the given key
            {make<SchemaCompilerAssertionDefines>(
                subcontext, entry.first, {},
                SchemaCompilerTargetType::Instance)}));
      }
    } else if (entry.second.is_array()) {
      std::set<JSON::String> properties;
      for (const auto &property : entry.second.as_array()) {
        assert(property.is_string());
        properties.emplace(property.to_string());
      }

      children.push_back(make<SchemaCompilerInternalDefinesAll>(
          subcontext, std::move(properties),
          // TODO: As an optimization, avoid this condition if the subschema
          // declares `required` and includes the given key
          {make<SchemaCompilerAssertionDefines>(
              subcontext, entry.first, {}, SchemaCompilerTargetType::Instance)},
          SchemaCompilerTargetType::Instance));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),
      type_condition(context, JSON::Type::Object))};
}

auto compiler_draft4_validation_enum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());

  if (context.value.size() == 1) {
    return {
        make<SchemaCompilerAssertionEqual>(context, context.value.front(), {},
                                           SchemaCompilerTargetType::Instance)};
  }

  std::set<JSON> options;
  for (const auto &option : context.value.as_array()) {
    options.insert(option);
  }

  return {make<SchemaCompilerAssertionEqualsAny>(
      context, std::move(options), SchemaCompilerTemplate{},
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_uniqueitems(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  if (!context.value.is_boolean() || !context.value.to_boolean()) {
    return {};
  }

  return {make<SchemaCompilerAssertionUnique>(
      context, SchemaCompilerValueNone{},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxlength(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) + 1},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minlength(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) - 1},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxitems(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) + 1},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minitems(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) - 1},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) + 1},
      type_condition(context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_integer() || context.value.is_integer_real());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.as_integer()) - 1},
      type_condition(context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maximum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{
      make<SchemaCompilerLogicalOr>(subcontext, SchemaCompilerValueNone{},
                                    {make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
                                     make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
                                    SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `minimum` is set to the same number, do
  // a single equality assertion

  assert(context.schema.is_object());
  if (context.schema.defines("exclusiveMaximum") &&
      context.schema.at("exclusiveMaximum").is_boolean() &&
      context.schema.at("exclusiveMaximum").to_boolean()) {
    return {make<SchemaCompilerAssertionLess>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionLessEqual>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_minimum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{
      make<SchemaCompilerLogicalOr>(subcontext, SchemaCompilerValueNone{},
                                    {make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
                                     make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
                                    SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `maximum` is set to the same number, do
  // a single equality assertion

  assert(context.schema.is_object());
  if (context.schema.defines("exclusiveMinimum") &&
      context.schema.at("exclusiveMinimum").is_boolean() &&
      context.schema.at("exclusiveMinimum").to_boolean()) {
    return {make<SchemaCompilerAssertionGreater>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionGreaterEqual>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_multipleof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  assert(context.value.is_positive());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate condition{
      make<SchemaCompilerLogicalOr>(subcontext, SchemaCompilerValueNone{},
                                    {make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
                                     make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
                                    SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionDivisible>(
      context, context.value, std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

} // namespace internal
#endif
