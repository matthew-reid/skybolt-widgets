// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#ifdef BUILD_WITH_SKYBOLT_REFLECT

#include "QtPropertyReflection.h"
#include "QtPropertyReflectionConversion.h"
#include "PropertyEditor.h"

#include <SkyboltReflect/Reflection.h>
#include <QVector3D>

namespace skybolt {

bool reflStructToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value)
{
	assert(type);
	const auto& reflContext = static_cast<const ReflQtValueUpdaterContext&>(valueUpdaterContext);

	QtStructValue structValue = value.value().value<QtStructValue>();

	// If Qt struct has not been created yet, create it now.
	// We assume that properties can not be added/removed from struct after creation, only updated.
	if (structValue.items.empty())
	{
		for (const auto& [childName, childProperty] : type->getProperties())
		{
			QtPropertyPtr property = createQtProperty(QString::fromStdString(childName), QVariant{});
			addMetadata(*property->value(), *childProperty);
			structValue.items.push_back(property);
		}
	}

	// Update child property for each struct item
	value.blockSignals(true); // Block signals while translating the property values to avoid emitting overall struct valueChanged signal until the end.
	int i = 0;
	for (const auto& [childName, childProperty] : type->getProperties())
	{
		const auto& childInstance = childProperty->getValue(reflContext.instance);

		ReflQtValueUpdaterContext childValueUpdaterContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = childInstance
		};

		QtPropertyPtr property = structValue.items[i];
		property->enabled = !childProperty->isReadOnly();
		bool success = reflValueToQt(childProperty->getType(), valueTranslators, childValueUpdaterContext, *property->value());
		if (success)
		{
			// Connect child property value change signals to parent property value change signal, so that when a child property value changes, the parent property valueChanged signal is emitted to notify the UI to update.
			QObject::connect(property->value().get(), &QtValue::valueChanged, &value, &QtValue::valueChanged, Qt::UniqueConnection);
		}
		++i;
	}
	value.blockSignals(false);

	value.setValue(QVariant::fromValue(structValue));

	return true;
}

bool reflStructFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value)
{
	assert(type);
	const auto& reflContext = static_cast<const ReflQtValueApplierContext&>(valueApplierContext);

	for (const auto& [childName, childProperty] : type->getProperties())
	{
		auto childInstance = childProperty->getValue(reflContext.instance);
		auto childInstanceSetter = [&] (const refl::Instance& instance) {
			childProperty->setValue(reflContext.instance, instance);
		};

		ReflQtValueApplierContext childValueApplierContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = childInstance,
		.instanceSetter = std::move(childInstanceSetter)
		};

		const auto& structValue = value.value().value<QtStructValue>();

		// Find the corresponding child QtProperty in the QtStructValue
		QtValuePtr childValue;
		{
			const auto& childQtPropertyIt = std::find_if(structValue.items.begin(), structValue.items.end(), [&childName] (const QtPropertyPtr& property) {
				return property->displayName.toStdString() == childName;
			});

			if (childQtPropertyIt == structValue.items.end())
			{
				// Skip this property if it can't be found in the QtStructValue
				continue;
			}

			childValue = (*childQtPropertyIt)->value();
		}

		reflValueFromQt(childProperty->getType(), valueTranslators, childValueApplierContext, *childValue);
	}

	reflContext.instanceSetter(reflContext.instance); // Notify that the instance has been updated after applying all child property values

	return true;
}

inline std::optional<refl::Instance> getOptionalValue(refl::TypeRegistry& registry, const refl::Instance& optionalInstance)
{
	auto accessor = optionalInstance.getType()->getContainerValueAccessor();
	if (!accessor) { return std::nullopt; }

	auto values = accessor->getValues(registry, optionalInstance);
	return values.empty() ? std::optional<refl::Instance>() : values.front();
}

inline void setOptionalValue(refl::Instance& optionalInstance, const std::optional<refl::Instance>& value)
{
	auto accessor = optionalInstance.getType()->getContainerValueAccessor();
	if (!accessor) { return; }

	accessor->setValues(optionalInstance, value ? std::vector<refl::Instance>({*value}) : std::vector<refl::Instance>());
}

bool optionalValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value)
{
	assert(type);

	auto accessor = type->getContainerValueAccessor();
	if (!accessor)
	{
		throw std::runtime_error("Type " + type->getName() + " does not have a container value accessor, which is required to create an optional value property.");
	}

	const auto& reflContext = static_cast<const ReflQtValueUpdaterContext&>(valueUpdaterContext);

	// Get child value type
	auto childType = reflContext.typeRegistry.getTypeByName(accessor->valueTypeName);
	if (!childType)
	{
		throw std::runtime_error("Type " + accessor->valueTypeName + " is not registered in the type registry.");
	}

	// Translate child value
	std::optional<refl::Instance> childInstance = getOptionalValue(reflContext.typeRegistry, reflContext.instance);

	value.blockSignals(true); // Block signals while setting up the optional value to avoid emitting valueChanged signal before the optional value is fully set up.

	QtOptionalValue optionalValue = value.value().value<QtOptionalValue>();
	optionalValue.present = childInstance.has_value();
	if (childInstance)
	{
		ReflQtValueUpdaterContext childValueUpdaterContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = *childInstance
		};
		reflValueToQt(childType, valueTranslators, childValueUpdaterContext, *optionalValue.value);
	}
	else if (!optionalValue.value->value().isValid())
	{
		// Optional is not present and child value has not been initialized yet. initialize it to the default value for the child data type.
		auto defaultInstance = childType->createDefaultInstance();
		if (!defaultInstance)
		{
			throw std::runtime_error("Unable to create default instance of type " + accessor->valueTypeName + " because the type is not constructable by the reflection system.");
		}

		ReflQtValueUpdaterContext childValueUpdaterContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = *defaultInstance
		};
		reflValueToQt(childType, valueTranslators, childValueUpdaterContext, *optionalValue.value);
	}

	value.blockSignals(false);

	// Ensure the item property change signal is connected to its parent vector property so that when an item property changes, the parent vector property will be notified and can apply the change to the reflected instance.
	QObject::connect(optionalValue.value.get(), &QtValue::valueChanged, &value, &QtValue::valueChanged, Qt::UniqueConnection);

	value.setValue(QVariant::fromValue(optionalValue));

	return true;
}

bool reflOptionalValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value)
{
	assert(type);

	auto accessor = type->getContainerValueAccessor();
	if (!accessor)
	{
		throw std::runtime_error("Type " + type->getName() + " does not have a container value accessor, which is required to create an optional value property.");
	}

	const auto& reflContext = static_cast<const ReflQtValueApplierContext&>(valueApplierContext);

	// Get child value type
	auto childType = reflContext.typeRegistry.getTypeByName(accessor->valueTypeName);
	if (!childType)
	{
		throw std::runtime_error("Type " + accessor->valueTypeName + " is not registered in the type registry.");
	}

	// Translate child value
	QtOptionalValue optionalValue = value.value().value<QtOptionalValue>();

	if (optionalValue.present)
	{
		std::unique_ptr<refl::Instance> childInstance = childType->createDefaultInstance();
		if (!childInstance)
		{
			return false; // Can't create instance for child type, so can't apply value
		}

		auto childInstanceSetter = [&] (const refl::Instance& instance) {
			setOptionalValue(reflContext.instance, instance);
			};

		ReflQtValueApplierContext childValueApplierContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = *childInstance,
		.instanceSetter = std::move(childInstanceSetter)
		};
		reflValueFromQt(childType, valueTranslators, childValueApplierContext, *optionalValue.value);
	}
	else
	{
		setOptionalValue(reflContext.instance, std::nullopt);
	}

	reflContext.instanceSetter(reflContext.instance); // Notify that the instance has been updated after applying the child property

	return true;
}

inline std::vector<refl::Instance> getVectorValues(refl::TypeRegistry& registry, const refl::Instance& vectorInstance)
{
	auto accessor = vectorInstance.getType()->getContainerValueAccessor();
	if (!accessor) { return {}; }

	return accessor->getValues(registry, vectorInstance);
}

inline void setVectorValues(refl::Instance& vectorInstance, const std::vector<refl::Instance>& values)
{
	auto accessor = vectorInstance.getType()->getContainerValueAccessor();
	if (!accessor) { return; }

	accessor->setValues(vectorInstance, values);
}

bool reflVectorValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value)
{
	assert(type);

	auto accessor = type->getContainerValueAccessor();
	if (!accessor)
	{
		throw std::runtime_error("Type " + type->getName() + " does not have a container value accessor, which is required to create a vector value property.");
	}

	const auto& reflContext = static_cast<const ReflQtValueUpdaterContext&>(valueUpdaterContext);

	// Get vector item type
	auto itemType = reflContext.typeRegistry.getTypeByName(accessor->valueTypeName);
	if (!itemType)
	{
		throw std::runtime_error("Type " + accessor->valueTypeName + " is not registered in the type registry.");
	}

	// Create default item
	std::shared_ptr<refl::Instance> defaultInstance = itemType->createDefaultInstance();
	if (!defaultInstance)
	{
		throw std::runtime_error("Cannot add new item to vector because item type " + itemType->getName() + " is not constructible by reflection system.");
	}

	QVariant qtItemDefaultValue;
	{
		ReflQtValueUpdaterContext childValueUpdaterContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = *defaultInstance
		};

		auto defaultValue = createQtValue(qtItemDefaultValue);
		reflValueToQt(itemType, valueTranslators, childValueUpdaterContext, *defaultValue);
		qtItemDefaultValue = defaultValue->value();
	}

	// Translate values
	std::vector<refl::Instance> valueInstances = getVectorValues(reflContext.typeRegistry, reflContext.instance);

	QtVectorValue vectorValue = value.value().value<QtVectorValue>();
	vectorValue.itemFactory = [qtItemDefaultValue]() { return createQtValue(qtItemDefaultValue); };
	vectorValue.items.resize(valueInstances.size());

	std::size_t i = 0;
	for (const auto& valueInstance : valueInstances)
	{
		QtValuePtr& itemValue = vectorValue.items[i];

		if (!itemValue)
		{
			itemValue = createQtValue(QVariant{});
		}

		ReflQtValueUpdaterContext childValueUpdaterContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = valueInstance
		};
		reflValueToQt(itemType, valueTranslators, childValueUpdaterContext, *itemValue);

		// Ensure the item property change signal is connected to its parent vector property so that when an item property changes, the parent vector property will be notified and can apply the change to the reflected instance.
		QObject::connect(itemValue.get(), &QtValue::valueChanged, &value, &QtValue::valueChanged, Qt::UniqueConnection);

		++i;
	}

	value.setValue(QVariant::fromValue(vectorValue));

	return true;
}

bool reflVectorValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value)
{
	assert(type);

	auto accessor = type->getContainerValueAccessor();
	if (!accessor)
	{
		throw std::runtime_error("Type " + type->getName() + " does not have a container value accessor, which is required to create a vector value property.");
	}

	const auto& reflContext = static_cast<const ReflQtValueApplierContext&>(valueApplierContext);

	// Get vector item type
	auto itemType = reflContext.typeRegistry.getTypeByName(accessor->valueTypeName);
	if (!itemType)
	{
		throw std::runtime_error("Type " + accessor->valueTypeName + " is not registered in the type registry.");
	}

	// Translate values
	const QtVectorValue& vectorValue = value.value().value<QtVectorValue>();

	std::vector<refl::Instance> valueInstances = getVectorValues(reflContext.typeRegistry, reflContext.instance);
	valueInstances.reserve(vectorValue.items.size());

	while (valueInstances.size() > vectorValue.items.size())
	{
		valueInstances.pop_back();
	}

	std::size_t i = 0;
	for (const QtValuePtr& itemValue : vectorValue.items)
	{
		refl::Instance* valueInstance;
		if (i < valueInstances.size())
		{
			valueInstance = &valueInstances[i];
		}
		else
		{
			std::unique_ptr<refl::Instance> childInstance = itemType->createDefaultInstance();
			if (!childInstance)
			{
				return false; // Can't create instance for item type, so can't apply value
			}
			valueInstances.push_back(*childInstance);
			valueInstance = &valueInstances.back();
		}

		auto childInstanceSetter = [&] (const refl::Instance& instance) {
			*valueInstance = instance;
			};

		ReflQtValueApplierContext childValueApplierContext{
		.typeRegistry = reflContext.typeRegistry,
		.instance = *valueInstance,
		.instanceSetter = std::move(childInstanceSetter)
		};
		reflValueFromQt(itemType, valueTranslators, childValueApplierContext, *itemValue);
		++i;
	}

	setVectorValues(reflContext.instance, valueInstances);
	reflContext.instanceSetter(reflContext.instance); // Notify that the instance has been updated after applying item values

	return true;
}


bool reflValueToQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueUpdaterContext& valueUpdaterContext, QtValue& value)
{
	assert(type);

	// Use a registered translator if one is available
	if (const auto& i = valueTranslators.find(type); i != valueTranslators.end())
	{
		const QtValueTranslator& translator = i->second;
		translator.updater(value, valueUpdaterContext);
		return true;
	}

	// No translator registered for this type, so we fallback to hardcoded translators
	if (auto accessor = type->getContainerValueAccessor(); accessor)
	{
		if (auto optionalValueTranslator = dynamic_cast<refl::StdOptionalValueAccessor*>(accessor.get()); optionalValueTranslator)
		{
				return optionalValueToQt(type, valueTranslators, valueUpdaterContext, value);
		}
		else if (auto vectorValueTranslator = dynamic_cast<refl::StdVectorValueAccessor*>(accessor.get()); vectorValueTranslator)
		{
			return reflVectorValueToQt(type, valueTranslators, valueUpdaterContext, value);
		}
	}
	else if (!type->getProperties().empty())
	{
		return reflStructToQt(type, valueTranslators, valueUpdaterContext, value);
	}

	return false;
}

bool reflValueFromQt(const refl::TypePtr& type, const ReflValueTranslatorMap& valueTranslators, const PropertiesModel::QtValueApplierContext& valueApplierContext, const QtValue& value)
{
	assert(type);

	// Use a registered translator if one is available
	if (const auto& i = valueTranslators.find(type); i != valueTranslators.end())
	{
		const QtValueTranslator& translator = i->second;
		translator.applier(value, valueApplierContext);
		return true;
	}
	
	// No translator registered for this type, so we fallback to hardcoded translators
	if (auto accessor = type->getContainerValueAccessor(); accessor)
	{
		if (auto optionalValueTranslator = dynamic_cast<refl::StdOptionalValueAccessor*>(accessor.get()); optionalValueTranslator)
		{
				return reflOptionalValueFromQt(type, valueTranslators, valueApplierContext, value);
		}
		else if (auto vectorValueTranslator = dynamic_cast<refl::StdVectorValueAccessor*>(accessor.get()); vectorValueTranslator)
		{
			return reflVectorValueFromQt(type, valueTranslators, valueApplierContext, value);
		}
	}
	else if (!type->getProperties().empty())
	{
		return reflStructFromQt(type, valueTranslators, valueApplierContext, value);
	}

	return false;
}

void addReflPropertiesToModel(refl::TypeRegistry& typeRegistry, PropertiesModel& model, const std::vector<refl::PropertyPtr>& properties, const ReflInstanceGetter& instanceGetter, const ReflValueTranslatorMap& valueTranslators)
{
	for (const refl::PropertyPtr& property : properties)
	{
		auto updater = [property, &typeRegistry, &valueTranslators, instanceGetter] (QtValue& value, const PropertiesModel::QtValueUpdaterContext& context) {

			if (auto instance = instanceGetter(); instance)
			{
				ReflQtValueUpdaterContext reflContext{
				.typeRegistry = typeRegistry,
				.instance = property->getValue(*instance)
				};
				reflValueToQt(property->getType(), valueTranslators, reflContext, value);
			}
		};

		auto applier = [property, &typeRegistry, &valueTranslators, instanceGetter] (const QtValue& value, const PropertiesModel::QtValueApplierContext& context) {

			if (auto instance = instanceGetter(); instance)
			{
				auto childInstance = property->getValue(*instance);
				auto childInstanceSetter = [&] (const refl::Instance& childInstance) {
					property->setValue(*instance, childInstance);
				};

				ReflQtValueApplierContext childValueApplierContext{
				.typeRegistry = typeRegistry,
				.instance = childInstance,
				.instanceSetter = std::move(childInstanceSetter)
				};
				reflValueFromQt(property->getType(), valueTranslators, childValueApplierContext, value);
			}
		};

		auto qtProperty = createQtProperty(QString::fromStdString(property->getName()), QVariant{});
		qtProperty->enabled = !property->isReadOnly();
		addMetadata(*qtProperty->value(), *property);
		updater(*qtProperty->value(), PropertiesModel::QtValueUpdaterContext{}); // Ensure the property value is initialized from the reflected instance

		model.addProperty(qtProperty, updater, applier, property->getCategory());
	}
}

template <typename KeyT, typename ValueT>
std::vector<ValueT> toValuesVector(const std::map<KeyT, ValueT>& m)
{
	std::vector<ValueT> r;
	for (const auto& i : m)
	{
		r.push_back(i.second);
	}
	return r;
}

QWidget* createReflValueInstanceEditor(QtValue* value, QWidget* parent, refl::TypeRegistry* typeRegistry, const ReflValueTranslatorMap& valueTranslators, const PropertyEditorWidgetFactoryMapPtr& factoryMap)
{
	auto instanceGetter = [value] () -> std::optional<refl::Instance> {
		auto instanceProperty = value->value().value<ReflInstanceVariant>();
		return instanceProperty.instance;
	};

	std::optional<refl::Instance> instance = instanceGetter();
	if (!instance) { return nullptr; }

	refl::TypePtr type = instance->getType();

	auto model = std::make_shared<PropertiesModel>();
	addReflPropertiesToModel(*typeRegistry, *model, toValuesVector(type->getProperties()), instanceGetter, valueTranslators);

	auto editor = new PropertyEditor(factoryMap, parent);
	editor->setModel(model);
	return editor;
}

} // namespace skybolt

#endif // BUILD_WITH_SKYBOLT_REFLECT