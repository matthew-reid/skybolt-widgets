// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#ifdef BUILD_WITH_SKYBOLT_REFLECT

#include "SkyboltWidgets/Property/PropertyEditorWidgetFactory.h"
#include "SkyboltWidgets/Property/QtProperty.h"
#include <SkyboltReflect/Reflection.h>

#include <functional>
#include <optional>

namespace skybolt {

struct QtValueTranslator
{
	PropertiesModel::QtValueUpdater updater; //!< Never null
	PropertiesModel::QtValueApplier applier; //!< Never null
};

struct ReflInstanceVariant
{
	std::optional<skybolt::refl::Instance> instance;
};

using ReflInstanceGetter = std::function<std::optional<skybolt::refl::Instance>()>;
using ReflInstanceSetter = std::function<void(const skybolt::refl::Instance&)>;

//! @param instanceSetter is null for read-only properties
using ReflValueTranslatorMap = std::map<skybolt::refl::TypePtr, QtValueTranslator>;
using ReflValueTranslatorMapPtr = std::shared_ptr<ReflValueTranslatorMap>;

bool reflStructToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value);
bool reflStructFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value);

bool reflOptionalValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value);
bool reflOptionalValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value);

bool reflVectorValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value);
bool reflVectorValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value);

bool reflValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value);
bool reflValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value);

void addReflPropertiesToModel(skybolt::refl::TypeRegistry& typeRegistry, PropertiesModel& model, const std::vector<skybolt::refl::PropertyPtr>& properties, const ReflInstanceGetter& instanceGetter, const ReflValueTranslatorMap& valueTranslators);

QWidget* createReflValueInstanceEditor(QtValue* value, QWidget* parent, refl::TypeRegistry* typeRegistry, const ReflValueTranslatorMap& valueTranslators, const PropertyEditorWidgetFactoryMapPtr& factoryMap);

} // namespace skybolt

Q_DECLARE_METATYPE(skybolt::ReflInstanceVariant);

#endif // BUILD_WITH_SKYBOLT_REFLECT