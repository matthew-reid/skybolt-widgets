// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include "SkyboltWidgets/Property/ContainerProperties.h"
#include "SkyboltWidgets/Property/QtPropertyMetadata.h"
#include "SkyboltWidgets/Util/QtTypeConversions.h"

#ifdef BUILD_WITH_SKYBOLT_REFLECT

Q_DECLARE_METATYPE(std::any);

namespace skybolt {

template <typename ReflValueT>
inline QVariant reflValueToQt(const ReflValueT& value)
{
	return QVariant::fromValue(value);
}

template <typename ReflValueT>
inline ReflValueT qtValueToRefl(const QVariant& value)
{
	return value.value<ReflValueT>();
}

template <>
inline QVariant reflValueToQt(const std::string& value)
{
	return QString::fromStdString(value);
}

template <>
inline std::string qtValueToRefl(const QVariant& value)
{
	return value.toString().toStdString();
}

template <typename ReflValueT>
QVariant valueInstanceToQt(const refl::Instance& valueInstance)
{
	if constexpr (std::is_same_v<ReflValueT, ReflInstanceVariant>)
	{
		return QVariant::fromValue(ReflInstanceVariant{ valueInstance });
	}
	else
	{
		const ReflValueT& value = valueInstance.cast<ReflValueT>();
		return reflValueToQt(value);
	}
}

template <typename ReflValueT>
std::optional<refl::Instance> qtToValueInstance(refl::TypeRegistry& typeRegistry, const QVariant value, const QVariant& defaultValue)
{
	if constexpr (std::is_same_v<ReflValueT, ReflInstanceVariant>)
	{
		std::optional<refl::Instance> optionalValueInstance = value.value<ReflInstanceVariant>().instance;
		if (!optionalValueInstance)
		{
			optionalValueInstance = defaultValue.value<ReflInstanceVariant>().instance;
		}
		return optionalValueInstance;
	}
	else
	{
		ReflValueT reflValue = qtValueToRefl<ReflValueT>(value);
		return refl::makeValueInstance(typeRegistry, reflValue);
	}
}

inline void addMetadata(QObject& qtObject, const refl::Property& reflProperty)
{
	for (const auto& [key, value] : reflProperty.getMetadataMap())
	{
		QVariant qtValue;
		if (value.type() == typeid(const char*))
		{
			auto cstr = std::any_cast<const char*>(value);
			qtValue = QString(cstr);
		}
		else if (value.type() == typeid(std::string))
		{
			qtValue = QString::fromStdString(std::any_cast<std::string>(value));
		}
		else if (value.type() == typeid(QString))
		{
			qtValue = std::any_cast<QString>(value);
		}
		else if (value.type() == typeid(bool))
		{
			qtValue = std::any_cast<bool>(value);
		}
		else if (value.type() == typeid(std::vector<std::string>))
		{
			qtValue = toQStringList(std::any_cast<std::vector<std::string>>(value));
		}
		else
		{
			// For other types, we fall back to storing the std::any directly as a QVariant using the registered metatype
			qtValue = QVariant::fromValue(value);
		}
		qtObject.setProperty(key.c_str(), qtValue);
	}
}

using DisplayNameRenderer = std::function<QString(const refl::Instance& reflValue, const QVariant& qtValue)>;
inline QString renderItemDisplayNameDefault(const refl::Instance& reflValue, const QVariant& qtValue)
{
	return qtValue.toString();
}

struct ReflQtValueUpdaterContext : PropertiesModel::QtValueUpdaterContext
{
	refl::TypeRegistry& typeRegistry;
	const refl::Instance& instance;
};

template <typename ReflValueT, typename QtValueT>
void updateQtValueFromRefl(QtValue& qtValue, const PropertiesModel::QtValueUpdaterContext& context)
{
	const auto& reflContext = static_cast<const ReflQtValueUpdaterContext&>(context);
	qtValue.setValue(valueInstanceToQt<ReflValueT>(reflContext.instance));
}

template <typename ReflValueT, typename QtValueT>
void updateQtValueFromReflWithDisplayName(QtValue& qtValue, const PropertiesModel::QtValueUpdaterContext& context, const DisplayNameRenderer& displayNameRenderer)
{
	const auto& reflContext = static_cast<const ReflQtValueUpdaterContext&>(context);
	qtValue.setValue(valueInstanceToQt<ReflValueT>(reflContext.instance));
	qtValue.setProperty(QtPropertyMetadataKeys::displayName, displayNameRenderer(reflContext.instance, qtValue.value()));
}

struct ReflQtValueApplierContext : PropertiesModel::QtValueApplierContext
{
	refl::TypeRegistry& typeRegistry;
	refl::Instance& instance; //!< existing instance, if amy
	ReflInstanceSetter instanceSetter; //!< function to set the instance to a new value
};

template <typename ReflValueT, typename QtValueT>
void applyQtValueToRefl(const QtValue& qtValue, const PropertiesModel::QtValueApplierContext& context)
{
	const auto& reflContext = static_cast<const ReflQtValueApplierContext&>(context);
	std::optional<refl::Instance> valueInstance = qtToValueInstance<ReflValueT>(reflContext.typeRegistry, qtValue.value(), QtValueT{});
	if (valueInstance)
	{
		reflContext.instanceSetter(*valueInstance);
	}
}

template <typename ReflValueT, typename QtValueT>
QtValueTranslator createReflValueTranslator()
{
	QtValueTranslator r;
	r.updater = &updateQtValueFromRefl<ReflValueT, QtValueT>;
	r.applier = &applyQtValueToRefl<ReflValueT, QtValueT>;
	return r;
}

template <typename ReflValueT, typename QtValueT>
QtValueTranslator createReflValueTranslatorWithDisplayName(DisplayNameRenderer displayNameRenderer)
{
	QtValueTranslator r;
	r.updater = [displayNameRenderer = std::move(displayNameRenderer)](QtValue& qtValue, const PropertiesModel::QtValueUpdaterContext& context) {
		updateQtValueFromReflWithDisplayName<ReflValueT, QtValueT>(qtValue, context, displayNameRenderer);
	};
	r.applier = &applyQtValueToRefl<ReflValueT, QtValueT>;
	return r;
}

inline ReflValueTranslatorMap createDefaultReflValueTranslators(skybolt::refl::TypeRegistry& typeRegistry)
{
	std::map<refl::TypePtr, QtValueTranslator> translators = {
		{ typeRegistry.getOrCreateType<std::string>(), createReflValueTranslator<std::string, QString>() },
		{ typeRegistry.getOrCreateType<bool>(), createReflValueTranslator<bool, bool>() },
		{ typeRegistry.getOrCreateType<int>(), createReflValueTranslator<int, int>() },
		{ typeRegistry.getOrCreateType<float>(), createReflValueTranslator<float, float>() },
		{ typeRegistry.getOrCreateType<double>(), createReflValueTranslator<double, double>() }
	};
	return translators;
}

} // namespace skybolt

#endif // BUILD_WITH_SKYBOLT_REFLECT