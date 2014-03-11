#include <Emodel.h>

EAPI Eo_Op EMODEL_OBJ_BASE_ID = EO_NOOP;

#define MY_CLASS EMODEL_OBJ_CLASS
#define MY_CLASS_NAME "emodel"

static void
_emodel_constructor(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_properties_get(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_property_get(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_property_set(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_load(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_unload(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_child_add(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_children_get(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_children_slice_get(*obj, void *class_data, va_list *list)
{
}

static void
_emodel_children_count_get(*obj, void *class_data, va_list *list)
{
}

static void
_class_constructor(Eo_Class *klass)
{
   const Eo_Op_Func_Description func_desc[] = {
      EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_CONSTRUCTOR), _emodel_constructor),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_PROPERTIES_GET), _emodel_properties_get),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_PROPERTY_GET), _emodel_property_get),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_PROPERTY_SET), _emodel_property_set),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_LOAD), _emodel_load),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_UNLOAD), _emodel_unload),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_CHILD_ADD), _emodel_child_add),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_CHILDREN_GET), _emodel_children_get),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_CHILDREN_SLICE_GET), _emodel_children_slice_get),
      EO_OP_FUNC(EMODEL_OBJ_ID(EMODEL_OBJ_SUB_ID_CHILDREN_COUNT_GET), _emodel_children_count_get),
      EO_OP_FUNC_SENTINEL
   };

   eo_class_funcs_set(klass, func_descs);
}

static const Eo_Op_Description op_desc[] = {
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_PROPERTIES_GET, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_PROPERTY_GET, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_PROPERTY_SET, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_LOAD, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_UNLOAD, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_CHILD_ADD, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_CHILDREN_GET, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_CHILDREN_SLICE_GET, ""),
      EO_OP_DESCRIPTION(EMODEL_OBJ_SUB_ID_CHILDREN_COUNT_GET, ""),
      EO_OP_DESCRIPTION_SENTINEL
};

static const Eo_Class_Description class_desc = {
     EO_VERSION,
     MY_CLASS_NAME,
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(&EMODEL_OBJ_BASE_ID, op_desc, EMODEL_OBJ_SUB_ID_LAST),
     NULL,
     0,
     _class_constructor,
     NULL
};

EO_DEFINE_CLASS(emodel_obj_class_get, &class_desc, EO_BASE_CLASS, NULL);
