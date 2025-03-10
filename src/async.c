/* async.c -- state management for asynchronous messages
 *
 * Copyright (C) 2010,2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

/**
 * @file async.c
 * @brief state management for asynchronous messages
 */

#include "coap2/coap_internal.h"

#ifndef WITHOUT_ASYNC

/* utlist-style macros for searching pairs in linked lists */
#define SEARCH_PAIR(head,out,field1,val1,field2,val2)   \
  SEARCH_PAIR2(head,out,field1,val1,field2,val2,next)

#define SEARCH_PAIR2(head,out,field1,val1,field2,val2,next)             \
  do {                                                                  \
    LL_FOREACH2(head,out,next) {                                        \
      if ((out)->field1 == (val1) && (out)->field2 == (val2)) break;    \
    }                                                                   \
} while(0)

coap_async_t *
coap_register_async(coap_context_t *context, coap_session_t *session,
                    coap_pdu_t *request, coap_tick_t delay) {
  coap_async_t *s;
  coap_mid_t mid = request->mid;
  size_t len;
  uint8_t *data;

  if (!COAP_PDU_IS_REQUEST(request))
    return NULL;

  SEARCH_PAIR(context->async_state,s,session,session,pdu->mid,mid);

  if (s != NULL) {
    coap_log(LOG_DEBUG,
         "asynchronous state for mid=0x%x already registered\n", mid);
    return NULL;
  }

  /* store information for handling the asynchronous task */
  s = (coap_async_t *)coap_malloc(sizeof(coap_async_t));
  if (!s) {
    coap_log(LOG_CRIT, "coap_register_async: insufficient memory\n");
    return NULL;
  }

  memset(s, 0, sizeof(coap_async_t));

  s->pdu = coap_pdu_duplicate(request, session, request->token_length,
                              request->token, NULL);
  if (s->pdu == NULL) {
    coap_free_async(context, s);
    coap_log(LOG_CRIT, "coap_register_async: insufficient memory\n");
    return NULL;
  }
  s->pdu->mid = mid; /* coap_pdu_duplicate() created one */

  if (coap_get_data(request, &len, &data)) {
    coap_add_data(s->pdu, len, data);
  }

  s->session = coap_session_reference( session );

  if (delay) {
    coap_ticks(&s->delay);
    s->delay += delay;
  }

  LL_PREPEND(context->async_state, s);

  return s;
}

void
coap_async_set_delay(coap_async_t *async, coap_tick_t delay) {
  assert(async != NULL);

  if (delay) {
    coap_ticks(&async->delay);
    async->delay += delay;
  }
  else
    async->delay = 0;
}


coap_async_t *
coap_find_async(coap_context_t *context, coap_session_t *session,
                coap_mid_t mid) {
  coap_async_t *tmp;
  SEARCH_PAIR(context->async_state,tmp,session,session,pdu->mid,mid);
  return tmp;
}

void
coap_free_async(coap_context_t *context, coap_async_t *s) {
  if (s) {
    LL_DELETE(context->async_state,s);
    if (s->session) {
      coap_session_release(s->session);
    }
    if (s->pdu) {
      coap_delete_pdu(s->pdu);
      s->pdu = NULL;
    }
    coap_free(s);
  }
}

void
coap_delete_all_async(coap_context_t *context) {
  coap_async_t *astate, *tmp;

  LL_FOREACH_SAFE(context->async_state, astate, tmp) {
    coap_free_async(context, astate);
  }
  context->async_state = NULL;
}

void
coap_async_set_app_data(coap_async_t *async, void *app_data) {
  async->appdata = app_data;
}

void *
coap_async_get_app_data(const coap_async_t *async) {
  return async->appdata;
}

#else
void does_not_exist(void);        /* make some compilers happy */
#endif /* WITHOUT_ASYNC */
