// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "ContainerProperties.h"

namespace skybolt {

QtPropertyPtr createOptionalQtProperty(const QString& displayName, const QVariant& optionalValue, bool present)
{
	auto property = std::make_shared<QtProperty>();
	property->displayName = displayName;

	QtOptionalValue opt;
	opt.present = present;
	opt.value = createQtValue(optionalValue);

	property->value()->setValue(QVariant::fromValue(opt));

	return property;
}

QtPropertyPtr createVectorQtProperty(const QString& displayName, const std::vector<QVariant>& values, const QVariant& itemDefaultValue)
{
	auto property = std::make_shared<QtProperty>();
	property->displayName = displayName;

	QtVectorValue children;
	children.itemFactory = [itemDefaultValue]() { return createQtValue(itemDefaultValue); };

	for (const auto& value : values)
	{
		auto childValue = createQtValue(value);
		children.items.push_back(childValue);
	}

	property->value()->setValue(QVariant::fromValue(children));
	return property;
}

} // namespace skybolt
