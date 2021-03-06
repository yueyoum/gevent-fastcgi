#include <Python.h>

#define ENSURE_LEN(req) if (end - buf < req) return PyErr_Format(PyExc_ValueError, "Buffer is %d byte(s) short", req)
#define PARSE_LEN(len) ENSURE_LEN(1); \
	len = *buf++; \
	if (len & 0x80) { \
		ENSURE_LEN(3); \
		len = ((len & 0x7f) << 24) + (buf[0] << 16) + (buf[1] << 8) + buf[2]; \
		buf += 3; \
	}

static PyObject *
py_unpack_pairs(PyObject *self, PyObject *args) {
	unsigned char *buf, *name, *value, *end;
	unsigned int blen, nlen, vlen;
	PyObject *result, *tuple;

	if (!PyArg_ParseTuple(args, "s#:unpack_pairs", &buf, &blen)) {
		return PyErr_Format(PyExc_ValueError, "Single string argument expected");
	}

	end = buf + blen;
	result = PyList_New(0);

	if (result) {
		while (buf < end) {
			PARSE_LEN(nlen);
			PARSE_LEN(vlen);
			ENSURE_LEN(nlen + vlen);
			name = buf;
			buf += nlen;
			value = buf;
			buf += vlen;
			tuple = Py_BuildValue("s#s#", name, nlen, value, vlen);
			if (tuple) {
				PyList_Append(result, tuple);
				Py_DECREF(tuple);
			} else {
				Py_XDECREF(result);
				return PyErr_Format(PyExc_RuntimeError, "Failed to allocate memory for next name/value tuple");
			}
		}
	}

	return result;
}

#define PACK_LEN(len) if (len > 127) { \
		*ptr++ = 0x80 + ((len >> 24) & 0xff); \
		*ptr++ = (len >> 16) & 0xff; \
		*ptr++ = (len >> 8) & 0xff; \
		*ptr++ = len & 0xff; \
	} else { \
		*ptr++ = len; \
	}

static PyObject *
py_pack_pair(PyObject *self, PyObject *args) {
	PyObject *result, *name, *value;
	unsigned char *buf, *ptr;
	int name_len, value_len, buf_len;

	if (!PyArg_ParseTuple(args, "s#s#:pack_pair", &name, &name_len, &value, &value_len)) {
		return NULL;
	}

	buf_len = name_len + value_len + (name_len > 127 ? 4 : 1) + (value_len > 127 ? 4 : 1);
	buf = ptr = (unsigned char*) PyMem_Malloc(buf_len);

	if (!buf) return PyErr_NoMemory();

	PACK_LEN(name_len);
	PACK_LEN(value_len);
	memcpy(ptr, name, name_len);
	memcpy(ptr + name_len, value, value_len);

	result = PyString_FromStringAndSize(buf, buf_len);
	PyMem_Free(buf);
	
	return result;
}

static PyMethodDef _methods[] = {
	{"unpack_pairs", py_unpack_pairs, METH_VARARGS},
	{"pack_pair", py_pack_pair, METH_VARARGS},
	{NULL, NULL}
};

PyMODINIT_FUNC
initspeedups(void) {
	Py_InitModule("speedups", _methods);
}
