/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: feff08cc6a667f071aa44151cfa83135e5c4cd9f */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_bsdiff_diff, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, old_file, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, new_file, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, diff_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_bsdiff_patch arginfo_bsdiff_diff


ZEND_FUNCTION(bsdiff_diff);
ZEND_FUNCTION(bsdiff_patch);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(bsdiff_diff, arginfo_bsdiff_diff)
	ZEND_FE(bsdiff_patch, arginfo_bsdiff_patch)
	ZEND_FE_END
};


static const zend_function_entry class_BsdiffException_methods[] = {
	ZEND_FE_END
};

static zend_class_entry *register_class_BsdiffException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "BsdiffException", class_BsdiffException_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);

	return class_entry;
}
