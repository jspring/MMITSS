/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "J2735SPATMESSAGE"
 * 	found in "module.asn1"
 * 	`asn1c -S/skeletons`
 */

#include <asn_internal.h>

#include "IntersectionState.h"

static int
memb_lanesCnt_constraint_1(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	long value;
	
	if(!sptr) {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	value = *(const long *)sptr;
	
	if((value >= 1 && value <= 255)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static int
memb_states_constraint_1(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	size_t size;
	
	if(!sptr) {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	/* Determine the number of elements */
	size = _A_CSEQUENCE_FROM_VOID(sptr)->count;
	
	if((size >= 1 && size <= 255)) {
		/* Perform validation of the inner elements */
		return td->check_constraints(td, sptr, ctfailcb, app_key);
	} else {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_TYPE_member_t asn_MBR_states_7[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_MovementState,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_states_tags_7[] = {
	(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_states_specs_7 = {
	sizeof(struct states),
	offsetof(struct states, _asn_ctx),
	0,	/* XER encoding is XMLDelimitedItemList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_states_7 = {
	"states",
	"states",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_states_tags_7,
	sizeof(asn_DEF_states_tags_7)
		/sizeof(asn_DEF_states_tags_7[0]) - 1, /* 1 */
	asn_DEF_states_tags_7,	/* Same as above */
	sizeof(asn_DEF_states_tags_7)
		/sizeof(asn_DEF_states_tags_7[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_states_7,
	1,	/* Single element */
	&asn_SPC_states_specs_7	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_IntersectionState_1[] = {
	{ ATF_POINTER, 1, offsetof(struct IntersectionState, name),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_DescriptiveName,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"name"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct IntersectionState, id),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_IntersectionID,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct IntersectionState, status),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_IntersectionStatusObject,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"status"
		},
	{ ATF_POINTER, 2, offsetof(struct IntersectionState, timeStamp),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_TimeMark,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"timeStamp"
		},
	{ ATF_POINTER, 1, offsetof(struct IntersectionState, lanesCnt),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NativeInteger,
		memb_lanesCnt_constraint_1,
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"lanesCnt"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct IntersectionState, states),
		(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
		0,
		&asn_DEF_states_7,
		memb_states_constraint_1,
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"states"
		},
	{ ATF_POINTER, 2, offsetof(struct IntersectionState, priority),
		(ASN_TAG_CLASS_CONTEXT | (6 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_SignalState,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"priority"
		},
	{ ATF_POINTER, 1, offsetof(struct IntersectionState, preempt),
		(ASN_TAG_CLASS_CONTEXT | (7 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_SignalState,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"preempt"
		},
};
static ber_tlv_tag_t asn_DEF_IntersectionState_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_IntersectionState_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* name at 44 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* id at 47 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* status at 52 */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* timeStamp at 54 */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 }, /* lanesCnt at 57 */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 5, 0, 0 }, /* states at 60 */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 6, 0, 0 }, /* priority at 64 */
    { (ASN_TAG_CLASS_CONTEXT | (7 << 2)), 7, 0, 0 } /* preempt at 66 */
};
static asn_SEQUENCE_specifics_t asn_SPC_IntersectionState_specs_1 = {
	sizeof(struct IntersectionState),
	offsetof(struct IntersectionState, _asn_ctx),
	asn_MAP_IntersectionState_tag2el_1,
	8,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	7,	/* Start extensions */
	9	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_IntersectionState = {
	"IntersectionState",
	"IntersectionState",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_IntersectionState_tags_1,
	sizeof(asn_DEF_IntersectionState_tags_1)
		/sizeof(asn_DEF_IntersectionState_tags_1[0]), /* 1 */
	asn_DEF_IntersectionState_tags_1,	/* Same as above */
	sizeof(asn_DEF_IntersectionState_tags_1)
		/sizeof(asn_DEF_IntersectionState_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_IntersectionState_1,
	8,	/* Elements count */
	&asn_SPC_IntersectionState_specs_1	/* Additional specs */
};

