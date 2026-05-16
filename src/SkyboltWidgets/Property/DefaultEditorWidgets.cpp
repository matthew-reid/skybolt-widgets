// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "DefaultEditorWidgets.h"
#include "ContainerProperties.h"
#include "EditorWidgets.h"
#include "PropertyEditor.h"
#include "QtPropertyMetadata.h"
#include "QtPropertyReflection.h"

namespace skybolt {

std::unique_ptr<PropertyEditorWidgetFactoryMap> createDefaultEditorWidgetFactoryMap(const DefaultEditorWidgetFactoryMapConfig& config)
{
	auto m = std::make_unique<PropertyEditorWidgetFactoryMap>(PropertyEditorWidgetFactoryMap{
		{ QMetaType::Type::QString, &createStringEditor },
		{ QMetaType::Type::Int, &createIntOrEnumEditor },
		{ QMetaType::Type::Float, &createDoubleEditor },
		{ QMetaType::Type::Double, &createDoubleEditor },
		{ QMetaType::Type::Bool, &createBoolEditor },
		{ QMetaType::Type::QDateTime, &createDateTimeEditor },
		{ QMetaType::Type::QVector3D, &createVector3DEditor }
	});

	// Note: we capture the raw pointer to the map in the lambdas below, but this is safe because moving a unique_ptr does not invalidate a raw pointer to it, and the lambdas are
	// stored inside the map which means the lifetime of the raw pointer is tied to the map.
	// See https://stackoverflow.com/questions/62132659/does-stdmove-invalidate-a-raw-pointer-obtained-from-unique-ptrget

	(*m)[qMetaTypeId<QtOptionalValue>()] = [factories = m.get()] (QtValue* value, QWidget* parent) {
		return createOptionalVariantEditor(*factories, value, parent);
	};
	(*m)[qMetaTypeId<QtVectorValue>()] = [factories = m.get(), listEditorIcons = config.listEditorIcons](QtValue* value, QWidget* parent) {
		return createVectorEditor(*factories, value, listEditorIcons, parent);
	};
	(*m)[qMetaTypeId<QtStructValue>()] = [factories = m.get()](QtValue* value, QWidget* parent) {
		return createStructEditor(*factories, value, parent);
	};
	return m;
}

} // namespace skybolt