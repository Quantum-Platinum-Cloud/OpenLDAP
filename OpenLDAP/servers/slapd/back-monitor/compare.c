/* compare.c - monitor backend compare routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-monitor/compare.c,v 1.15.2.4 2004/03/18 00:56:29 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2004 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <slap.h>
#include "back-monitor.h"

int
monitor_back_compare( struct slap_op *op, struct slap_rep *rs)
{
	struct monitorinfo      *mi = 
		(struct monitorinfo *) op->o_bd->be_private;
	Entry           *e, *matched = NULL;
	Attribute	*a;

	/* get entry with reader lock */
	monitor_cache_dn2entry( op, &op->o_req_ndn, &e, &matched );
	if ( e == NULL ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		if ( matched ) {
			rs->sr_matched = matched->e_dn;
		}
		send_ldap_result( op, rs );
		if ( matched ) {
			monitor_cache_release( mi, matched );
			rs->sr_matched = NULL;
		}

		return( 0 );
	}

	rs->sr_err = access_allowed( op, e, op->oq_compare.rs_ava->aa_desc,
			&op->oq_compare.rs_ava->aa_value, ACL_COMPARE, NULL );
	if ( !rs->sr_err ) {
		rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		goto return_results;
	}

	rs->sr_err = LDAP_NO_SUCH_ATTRIBUTE;

	for ( a = attrs_find( e->e_attrs, op->oq_compare.rs_ava->aa_desc );
			a != NULL;
			a = attrs_find( a->a_next, op->oq_compare.rs_ava->aa_desc )) {
		rs->sr_err = LDAP_COMPARE_FALSE;

		if ( value_find_ex( op->oq_compare.rs_ava->aa_desc,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
			a->a_nvals, &op->oq_compare.rs_ava->aa_value,
			op->o_tmpmemctx ) == 0 )
		{
			rs->sr_err = LDAP_COMPARE_TRUE;
			break;
		}
	}

return_results:;
	send_ldap_result( op, rs );
	if ( rs->sr_err == LDAP_COMPARE_FALSE
			|| rs->sr_err == LDAP_COMPARE_TRUE ) {
		rs->sr_err = LDAP_SUCCESS;
	}

	monitor_cache_release( mi, e );

	return( rs->sr_err );
}
