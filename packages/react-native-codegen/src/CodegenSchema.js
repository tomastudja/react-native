/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict
 * @format
 */

'use strict';

export type PlatformType = 'iOS' | 'android';

export type CommandsFunctionTypeAnnotation = $ReadOnly<{|
  type: 'FunctionTypeAnnotation',
  params: $ReadOnlyArray<CommandsFunctionTypeParamAnnotation>,
|}>;

export type CommandsFunctionTypeParamAnnotation = $ReadOnly<{|
  name: string,
  typeAnnotation: CommandsTypeAnnotation,
|}>;

export type CommandsTypeAnnotation =
  | ReservedFunctionValueTypeAnnotation
  | BooleanTypeAnnotation
  | Int32TypeAnnotation
  | DoubleTypeAnnotation
  | FloatTypeAnnotation
  | StringTypeAnnotation;

type ReservedFunctionValueTypeAnnotation = $ReadOnly<{|
  type: 'ReservedFunctionValueTypeAnnotation',
  name: ReservedFunctionValueTypeName,
|}>;

export type DoubleTypeAnnotation = $ReadOnly<{|
  type: 'DoubleTypeAnnotation',
|}>;

export type FloatTypeAnnotation = $ReadOnly<{|
  type: 'FloatTypeAnnotation',
|}>;

export type BooleanTypeAnnotation = $ReadOnly<{|
  type: 'BooleanTypeAnnotation',
|}>;

export type Int32TypeAnnotation = $ReadOnly<{|
  type: 'Int32TypeAnnotation',
|}>;

export type StringTypeAnnotation = $ReadOnly<{|
  type: 'StringTypeAnnotation',
|}>;

export type EventObjectPropertyType =
  | $ReadOnly<{|
      type: 'BooleanTypeAnnotation',
      name: string,
      optional: boolean,
    |}>
  | $ReadOnly<{|
      type: 'StringTypeAnnotation',
      name: string,
      optional: boolean,
    |}>
  | $ReadOnly<{|
      type: 'DoubleTypeAnnotation',
      name: string,
      optional: boolean,
    |}>
  | $ReadOnly<{|
      type: 'FloatTypeAnnotation',
      name: string,
      optional: boolean,
    |}>
  | $ReadOnly<{|
      type: 'Int32TypeAnnotation',
      name: string,
      optional: boolean,
    |}>
  | $ReadOnly<{|
      type: 'StringEnumTypeAnnotation',
      name: string,
      optional: boolean,
      options: $ReadOnlyArray<{|
        name: string,
      |}>,
    |}>
  | $ReadOnly<{|
      type: 'ObjectTypeAnnotation',
      name: string,
      optional: boolean,
      properties: $ReadOnlyArray<EventObjectPropertyType>,
    |}>;

type PropTypeTypeAnnotation =
  | $ReadOnly<{|
      type: 'BooleanTypeAnnotation',
      default: boolean | null,
    |}>
  | $ReadOnly<{|
      type: 'StringTypeAnnotation',
      default: string | null,
    |}>
  | $ReadOnly<{|
      type: 'DoubleTypeAnnotation',
      default: number,
    |}>
  | $ReadOnly<{|
      type: 'FloatTypeAnnotation',
      default: number | null,
    |}>
  | $ReadOnly<{|
      type: 'Int32TypeAnnotation',
      default: number,
    |}>
  | $ReadOnly<{|
      type: 'StringEnumTypeAnnotation',
      default: string,
      options: $ReadOnlyArray<{|
        name: string,
      |}>,
    |}>
  | $ReadOnly<{|
      type: 'Int32EnumTypeAnnotation',
      default: number,
      options: $ReadOnlyArray<{|
        value: number,
      |}>,
    |}>
  | $ReadOnly<{|
      type: 'ReservedPropTypeAnnotation',
      name:
        | 'ColorPrimitive'
        | 'ImageSourcePrimitive'
        | 'PointPrimitive'
        | 'EdgeInsetsPrimitive',
    |}>
  | $ReadOnly<{|
      type: 'ObjectTypeAnnotation',
      properties: $ReadOnlyArray<PropTypeShape>,
    |}>
  | $ReadOnly<{|
      type: 'ArrayTypeAnnotation',
      elementType:
        | $ReadOnly<{|
            type: 'BooleanTypeAnnotation',
          |}>
        | $ReadOnly<{|
            type: 'StringTypeAnnotation',
          |}>
        | $ReadOnly<{|
            type: 'DoubleTypeAnnotation',
          |}>
        | $ReadOnly<{|
            type: 'FloatTypeAnnotation',
          |}>
        | $ReadOnly<{|
            type: 'Int32TypeAnnotation',
          |}>
        | $ReadOnly<{|
            type: 'StringEnumTypeAnnotation',
            default: string,
            options: $ReadOnlyArray<{|
              name: string,
            |}>,
          |}>
        | $ReadOnly<{|
            type: 'ObjectTypeAnnotation',
            properties: $ReadOnlyArray<PropTypeShape>,
          |}>
        | $ReadOnly<{|
            type: 'ReservedPropTypeAnnotation',
            name:
              | 'ColorPrimitive'
              | 'ImageSourcePrimitive'
              | 'PointPrimitive'
              | 'EdgeInsetsPrimitive',
          |}>
        | $ReadOnly<{|
            type: 'ArrayTypeAnnotation',
            elementType: $ReadOnly<{|
              type: 'ObjectTypeAnnotation',
              properties: $ReadOnlyArray<PropTypeShape>,
            |}>,
          |}>,
    |}>;

export type PropTypeShape = $ReadOnly<{|
  name: string,
  optional: boolean,
  typeAnnotation: PropTypeTypeAnnotation,
|}>;

export type EventTypeShape = $ReadOnly<{|
  name: string,
  bubblingType: 'direct' | 'bubble',
  optional: boolean,
  paperTopLevelNameDeprecated?: string,
  typeAnnotation: $ReadOnly<{|
    type: 'EventTypeAnnotation',
    argument?: $ReadOnly<{|
      type: 'ObjectTypeAnnotation',
      properties: $ReadOnlyArray<EventObjectPropertyType>,
    |}>,
  |}>,
|}>;

export type CommandTypeShape = $ReadOnly<{|
  name: string,
  optional: boolean,
  typeAnnotation: CommandsFunctionTypeAnnotation,
|}>;

export type OptionsShape = $ReadOnly<{|
  interfaceOnly?: boolean,

  // Use for components with no current paper rename in progress
  // Does not check for new name
  paperComponentName?: string,

  // Use for components that are not used on other platforms.
  excludedPlatforms?: $ReadOnlyArray<PlatformType>,

  // Use for components currently being renamed in paper
  // Will use new name if it is available and fallback to this name
  paperComponentNameDeprecated?: string,
|}>;

export type ExtendsPropsShape = $ReadOnly<{|
  type: 'ReactNativeBuiltInType',
  knownTypeName: 'ReactNativeCoreViewProps',
|}>;

export type ComponentShape = $ReadOnly<{|
  ...OptionsShape,
  extendsProps: $ReadOnlyArray<ExtendsPropsShape>,
  events: $ReadOnlyArray<EventTypeShape>,
  props: $ReadOnlyArray<PropTypeShape>,
  commands: $ReadOnlyArray<CommandTypeShape>,
|}>;

export type SchemaType = $ReadOnly<{|
  modules: $ReadOnly<{
    [module: string]: $ReadOnly<{|
      components?: $ReadOnly<{[component: string]: ComponentShape, ...}>,
      nativeModules?: $ReadOnly<{
        [nativeModule: string]: NativeModuleSchema,
        ...,
      }>,
    |}>,
    ...,
  }>,
|}>;

/**
 * NativeModule Types
 */
export type NativeModuleSchema = $ReadOnly<{|
  // We only support aliases to Objects
  aliases: $ReadOnly<NativeModuleAliasMap>,
  properties: $ReadOnlyArray<NativeModulePropertySchema>,
|}>;

export type NativeModuleAliasMap = {
  [aliasName: string]: NativeModuleObjectTypeAnnotation,
};

export type NativeModulePropertySchema = $ReadOnly<{|
  name: string,
  optional: boolean,
  typeAnnotation: NativeModuleFunctionTypeAnnotation,
|}>;

export type NativeModuleMethodParamSchema = $ReadOnly<{|
  name: string,
  optional: boolean,
  typeAnnotation: NativeModuleParamTypeAnnotation,
|}>;

export type NativeModuleFunctionTypeAnnotation = $ReadOnly<{|
  type: 'FunctionTypeAnnotation',
  params: $ReadOnlyArray<NativeModuleMethodParamSchema>,
  returnTypeAnnotation: NativeModuleReturnTypeAnnotation,
  nullable: boolean,
|}>;

export type NativeModuleObjectTypeAnnotation = $ReadOnly<{|
  type: 'ObjectTypeAnnotation',
  properties: $ReadOnlyArray<
    $ReadOnly<{|
      optional: boolean,
      name: string,
      typeAnnotation: NativeModuleParamTypeAnnotation,
    |}>,
  >,
  nullable: boolean,
|}>;

export type NativeModuleBaseTypeAnnotation =
  | $ReadOnly<{|
      type: 'StringTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'NumberTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'Int32TypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'DoubleTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'FloatTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'BooleanTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'GenericObjectTypeAnnotation',
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'ReservedFunctionValueTypeAnnotation',
      name: ReservedFunctionValueTypeName,
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'ArrayTypeAnnotation',
      /**
       * TODO(T72031674): Migrate all our NativeModule specs to not use
       * invalid Array ElementTypes. Then, make the elementType required.
       */
      elementType?: NativeModuleBaseTypeAnnotation,
      nullable: boolean,
    |}>
  | $ReadOnly<{|
      type: 'TypeAliasTypeAnnotation',
      name: string,
      nullable: boolean,
    |}>
  | NativeModuleObjectTypeAnnotation;

export type NativeModuleParamOnlyTypeAnnotation = NativeModuleFunctionTypeAnnotation;
export type NativeModuleReturnOnlyTypeAnnotation =
  | $ReadOnly<{|
      nullable: boolean,
      type: 'GenericPromiseTypeAnnotation',
    |}>
  | $ReadOnly<{|
      nullable: boolean,
      type: 'VoidTypeAnnotation',
    |}>;

export type NativeModuleParamTypeAnnotation =
  | NativeModuleBaseTypeAnnotation
  | NativeModuleParamOnlyTypeAnnotation;

export type NativeModuleReturnTypeAnnotation =
  | NativeModuleBaseTypeAnnotation
  | NativeModuleReturnOnlyTypeAnnotation;

export type NativeModuleTypeAnnotation =
  | NativeModuleBaseTypeAnnotation
  | NativeModuleParamOnlyTypeAnnotation
  | NativeModuleReturnOnlyTypeAnnotation;

export type ReservedFunctionValueTypeName = 'RootTag'; // Union with more custom types.
