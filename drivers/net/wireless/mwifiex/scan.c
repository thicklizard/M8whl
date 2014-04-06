/*
 * Marvell Wireless LAN device driver: scan ioctl and command handling
 *
 * Copyright (C) 2011, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#include "decl.h"
#include "ioctl.h"
#include "util.h"
#include "fw.h"
#include "main.h"
#include "11n.h"
#include "cfg80211.h"

#define MWIFIEX_MAX_CHANNELS_PER_SPECIFIC_SCAN   14

#define MWIFIEX_CHANNELS_PER_SCAN_CMD            4

#define CHAN_TLV_MAX_SIZE  (sizeof(struct mwifiex_ie_types_header)         \
				+ (MWIFIEX_MAX_CHANNELS_PER_SPECIFIC_SCAN     \
				*sizeof(struct mwifiex_chan_scan_param_set)))

#define RATE_TLV_MAX_SIZE   (sizeof(struct mwifiex_ie_types_rates_param_set) \
				+ HOSTCMD_SUPPORTED_RATES)

#define WILDCARD_SSID_TLV_MAX_SIZE  \
	(MWIFIEX_MAX_SSID_LIST_LENGTH *					\
		(sizeof(struct mwifiex_ie_types_wildcard_ssid_params)	\
			+ IEEE80211_MAX_SSID_LEN))

#define MAX_SCAN_CFG_ALLOC (sizeof(struct mwifiex_scan_cmd_config)        \
				+ sizeof(struct mwifiex_ie_types_num_probes)   \
				+ sizeof(struct mwifiex_ie_types_htcap)       \
				+ CHAN_TLV_MAX_SIZE                 \
				+ RATE_TLV_MAX_SIZE                 \
				+ WILDCARD_SSID_TLV_MAX_SIZE)


union mwifiex_scan_cmd_config_tlv {
	
	struct mwifiex_scan_cmd_config config;
	
	u8 config_alloc_buf[MAX_SCAN_CFG_ALLOC];
};

enum cipher_suite {
	CIPHER_SUITE_TKIP,
	CIPHER_SUITE_CCMP,
	CIPHER_SUITE_MAX
};
static u8 mwifiex_wpa_oui[CIPHER_SUITE_MAX][4] = {
	{ 0x00, 0x50, 0xf2, 0x02 },	
	{ 0x00, 0x50, 0xf2, 0x04 },	
};
static u8 mwifiex_rsn_oui[CIPHER_SUITE_MAX][4] = {
	{ 0x00, 0x0f, 0xac, 0x02 },	
	{ 0x00, 0x0f, 0xac, 0x04 },	
};

static u8
mwifiex_search_oui_in_ie(struct ie_body *iebody, u8 *oui)
{
	u8 count;

	count = iebody->ptk_cnt[0];

	while (count) {
		if (!memcmp(iebody->ptk_body, oui, sizeof(iebody->ptk_body)))
			return MWIFIEX_OUI_PRESENT;

		--count;
		if (count)
			iebody = (struct ie_body *) ((u8 *) iebody +
						sizeof(iebody->ptk_body));
	}

	pr_debug("info: %s: OUI is not found in PTK\n", __func__);
	return MWIFIEX_OUI_NOT_PRESENT;
}

static u8
mwifiex_is_rsn_oui_present(struct mwifiex_bssdescriptor *bss_desc, u32 cipher)
{
	u8 *oui;
	struct ie_body *iebody;
	u8 ret = MWIFIEX_OUI_NOT_PRESENT;

	if (((bss_desc->bcn_rsn_ie) && ((*(bss_desc->bcn_rsn_ie)).
					ieee_hdr.element_id == WLAN_EID_RSN))) {
		iebody = (struct ie_body *)
			 (((u8 *) bss_desc->bcn_rsn_ie->data) +
			  RSN_GTK_OUI_OFFSET);
		oui = &mwifiex_rsn_oui[cipher][0];
		ret = mwifiex_search_oui_in_ie(iebody, oui);
		if (ret)
			return ret;
	}
	return ret;
}

static u8
mwifiex_is_wpa_oui_present(struct mwifiex_bssdescriptor *bss_desc, u32 cipher)
{
	u8 *oui;
	struct ie_body *iebody;
	u8 ret = MWIFIEX_OUI_NOT_PRESENT;

	if (((bss_desc->bcn_wpa_ie) &&
	     ((*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id ==
	      WLAN_EID_WPA))) {
		iebody = (struct ie_body *) bss_desc->bcn_wpa_ie->data;
		oui = &mwifiex_wpa_oui[cipher][0];
		ret = mwifiex_search_oui_in_ie(iebody, oui);
		if (ret)
			return ret;
	}
	return ret;
}

s32
mwifiex_ssid_cmp(struct cfg80211_ssid *ssid1, struct cfg80211_ssid *ssid2)
{
	if (!ssid1 || !ssid2 || (ssid1->ssid_len != ssid2->ssid_len))
		return -1;
	return memcmp(ssid1->ssid, ssid2->ssid, ssid1->ssid_len);
}

static bool
mwifiex_is_bss_wapi(struct mwifiex_private *priv,
		    struct mwifiex_bssdescriptor *bss_desc)
{
	if (priv->sec_info.wapi_enabled &&
	    (bss_desc->bcn_wapi_ie &&
	     ((*(bss_desc->bcn_wapi_ie)).ieee_hdr.element_id ==
			WLAN_EID_BSS_AC_ACCESS_DELAY))) {
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_no_sec(struct mwifiex_private *priv,
		      struct mwifiex_bssdescriptor *bss_desc)
{
	if (!priv->sec_info.wep_enabled && !priv->sec_info.wpa_enabled &&
	    !priv->sec_info.wpa2_enabled && ((!bss_desc->bcn_wpa_ie) ||
		((*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id !=
		 WLAN_EID_WPA)) &&
	    ((!bss_desc->bcn_rsn_ie) ||
		((*(bss_desc->bcn_rsn_ie)).ieee_hdr.element_id !=
		 WLAN_EID_RSN)) &&
	    !priv->sec_info.encryption_mode && !bss_desc->privacy) {
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_static_wep(struct mwifiex_private *priv,
			  struct mwifiex_bssdescriptor *bss_desc)
{
	if (priv->sec_info.wep_enabled && !priv->sec_info.wpa_enabled &&
	    !priv->sec_info.wpa2_enabled && bss_desc->privacy) {
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_wpa(struct mwifiex_private *priv,
		   struct mwifiex_bssdescriptor *bss_desc)
{
	if (!priv->sec_info.wep_enabled && priv->sec_info.wpa_enabled &&
	    !priv->sec_info.wpa2_enabled && ((bss_desc->bcn_wpa_ie) &&
	    ((*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id == WLAN_EID_WPA))
	 ) {
		dev_dbg(priv->adapter->dev, "info: %s: WPA:"
			" wpa_ie=%#x wpa2_ie=%#x WEP=%s WPA=%s WPA2=%s "
			"EncMode=%#x privacy=%#x\n", __func__,
			(bss_desc->bcn_wpa_ie) ?
			(*(bss_desc->bcn_wpa_ie)).
			vend_hdr.element_id : 0,
			(bss_desc->bcn_rsn_ie) ?
			(*(bss_desc->bcn_rsn_ie)).
			ieee_hdr.element_id : 0,
			(priv->sec_info.wep_enabled) ? "e" : "d",
			(priv->sec_info.wpa_enabled) ? "e" : "d",
			(priv->sec_info.wpa2_enabled) ? "e" : "d",
			priv->sec_info.encryption_mode,
			bss_desc->privacy);
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_wpa2(struct mwifiex_private *priv,
		    struct mwifiex_bssdescriptor *bss_desc)
{
	if (!priv->sec_info.wep_enabled &&
	    !priv->sec_info.wpa_enabled &&
	    priv->sec_info.wpa2_enabled &&
	    ((bss_desc->bcn_rsn_ie) &&
	     ((*(bss_desc->bcn_rsn_ie)).ieee_hdr.element_id == WLAN_EID_RSN))) {
		dev_dbg(priv->adapter->dev, "info: %s: WPA2: "
			" wpa_ie=%#x wpa2_ie=%#x WEP=%s WPA=%s WPA2=%s "
			"EncMode=%#x privacy=%#x\n", __func__,
			(bss_desc->bcn_wpa_ie) ?
			(*(bss_desc->bcn_wpa_ie)).
			vend_hdr.element_id : 0,
			(bss_desc->bcn_rsn_ie) ?
			(*(bss_desc->bcn_rsn_ie)).
			ieee_hdr.element_id : 0,
			(priv->sec_info.wep_enabled) ? "e" : "d",
			(priv->sec_info.wpa_enabled) ? "e" : "d",
			(priv->sec_info.wpa2_enabled) ? "e" : "d",
			priv->sec_info.encryption_mode,
			bss_desc->privacy);
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_adhoc_aes(struct mwifiex_private *priv,
			 struct mwifiex_bssdescriptor *bss_desc)
{
	if (!priv->sec_info.wep_enabled && !priv->sec_info.wpa_enabled &&
	    !priv->sec_info.wpa2_enabled &&
	    ((!bss_desc->bcn_wpa_ie) ||
	     ((*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id != WLAN_EID_WPA)) &&
	    ((!bss_desc->bcn_rsn_ie) ||
	     ((*(bss_desc->bcn_rsn_ie)).ieee_hdr.element_id != WLAN_EID_RSN)) &&
	    !priv->sec_info.encryption_mode && bss_desc->privacy) {
		return true;
	}
	return false;
}

static bool
mwifiex_is_bss_dynamic_wep(struct mwifiex_private *priv,
			   struct mwifiex_bssdescriptor *bss_desc)
{
	if (!priv->sec_info.wep_enabled && !priv->sec_info.wpa_enabled &&
	    !priv->sec_info.wpa2_enabled &&
	    ((!bss_desc->bcn_wpa_ie) ||
	     ((*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id != WLAN_EID_WPA)) &&
	    ((!bss_desc->bcn_rsn_ie) ||
	     ((*(bss_desc->bcn_rsn_ie)).ieee_hdr.element_id != WLAN_EID_RSN)) &&
	    priv->sec_info.encryption_mode && bss_desc->privacy) {
		dev_dbg(priv->adapter->dev, "info: %s: dynamic "
			"WEP: wpa_ie=%#x wpa2_ie=%#x "
			"EncMode=%#x privacy=%#x\n",
			__func__,
			(bss_desc->bcn_wpa_ie) ?
			(*(bss_desc->bcn_wpa_ie)).
			vend_hdr.element_id : 0,
			(bss_desc->bcn_rsn_ie) ?
			(*(bss_desc->bcn_rsn_ie)).
			ieee_hdr.element_id : 0,
			priv->sec_info.encryption_mode,
			bss_desc->privacy);
		return true;
	}
	return false;
}

static s32
mwifiex_is_network_compatible(struct mwifiex_private *priv,
			      struct mwifiex_bssdescriptor *bss_desc, u32 mode)
{
	struct mwifiex_adapter *adapter = priv->adapter;

	bss_desc->disable_11n = false;

	
	if (priv->media_connected &&
	    (priv->bss_mode == NL80211_IFTYPE_STATION) &&
	    (bss_desc->bss_mode == NL80211_IFTYPE_STATION))
		return 0;

	if (priv->wps.session_enable) {
		dev_dbg(adapter->dev,
			"info: return success directly in WPS period\n");
		return 0;
	}

	if (mwifiex_is_bss_wapi(priv, bss_desc)) {
		dev_dbg(adapter->dev, "info: return success for WAPI AP\n");
		return 0;
	}

	if (bss_desc->bss_mode == mode) {
		if (mwifiex_is_bss_no_sec(priv, bss_desc)) {
			
			return 0;
		} else if (mwifiex_is_bss_static_wep(priv, bss_desc)) {
			
			dev_dbg(adapter->dev, "info: Disable 11n in WEP mode.\n");
			bss_desc->disable_11n = true;
			return 0;
		} else if (mwifiex_is_bss_wpa(priv, bss_desc)) {
			
			if (((priv->adapter->config_bands & BAND_GN ||
			      priv->adapter->config_bands & BAND_AN) &&
			     bss_desc->bcn_ht_cap) &&
			    !mwifiex_is_wpa_oui_present(bss_desc,
							 CIPHER_SUITE_CCMP)) {

				if (mwifiex_is_wpa_oui_present
						(bss_desc, CIPHER_SUITE_TKIP)) {
					dev_dbg(adapter->dev,
						"info: Disable 11n if AES "
						"is not supported by AP\n");
					bss_desc->disable_11n = true;
				} else {
					return -1;
				}
			}
			return 0;
		} else if (mwifiex_is_bss_wpa2(priv, bss_desc)) {
			
			if (((priv->adapter->config_bands & BAND_GN ||
			      priv->adapter->config_bands & BAND_AN) &&
			     bss_desc->bcn_ht_cap) &&
			    !mwifiex_is_rsn_oui_present(bss_desc,
							CIPHER_SUITE_CCMP)) {

				if (mwifiex_is_rsn_oui_present
						(bss_desc, CIPHER_SUITE_TKIP)) {
					dev_dbg(adapter->dev,
						"info: Disable 11n if AES "
						"is not supported by AP\n");
					bss_desc->disable_11n = true;
				} else {
					return -1;
				}
			}
			return 0;
		} else if (mwifiex_is_bss_adhoc_aes(priv, bss_desc)) {
			
			return 0;
		} else if (mwifiex_is_bss_dynamic_wep(priv, bss_desc)) {
			
			return 0;
		}

		
		dev_dbg(adapter->dev,
			"info: %s: failed: wpa_ie=%#x wpa2_ie=%#x WEP=%s "
			"WPA=%s WPA2=%s EncMode=%#x privacy=%#x\n", __func__,
			(bss_desc->bcn_wpa_ie) ?
			(*(bss_desc->bcn_wpa_ie)).vend_hdr.element_id : 0,
			(bss_desc->bcn_rsn_ie) ?
			(*(bss_desc->bcn_rsn_ie)).ieee_hdr.element_id : 0,
			(priv->sec_info.wep_enabled) ? "e" : "d",
			(priv->sec_info.wpa_enabled) ? "e" : "d",
			(priv->sec_info.wpa2_enabled) ? "e" : "d",
			priv->sec_info.encryption_mode, bss_desc->privacy);
		return -1;
	}

	
	return -1;
}

static void
mwifiex_scan_create_channel_list(struct mwifiex_private *priv,
				 const struct mwifiex_user_scan_cfg
							*user_scan_in,
				 struct mwifiex_chan_scan_param_set
							*scan_chan_list,
				 u8 filtered_scan)
{
	enum ieee80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *ch;
	struct mwifiex_adapter *adapter = priv->adapter;
	int chan_idx = 0, i;

	for (band = 0; (band < IEEE80211_NUM_BANDS) ; band++) {

		if (!priv->wdev->wiphy->bands[band])
			continue;

		sband = priv->wdev->wiphy->bands[band];

		for (i = 0; (i < sband->n_channels) ; i++) {
			ch = &sband->channels[i];
			if (ch->flags & IEEE80211_CHAN_DISABLED)
				continue;
			scan_chan_list[chan_idx].radio_type = band;

			if (user_scan_in &&
			    user_scan_in->chan_list[0].scan_time)
				scan_chan_list[chan_idx].max_scan_time =
					cpu_to_le16((u16) user_scan_in->
					chan_list[0].scan_time);
			else if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
				scan_chan_list[chan_idx].max_scan_time =
					cpu_to_le16(adapter->passive_scan_time);
			else
				scan_chan_list[chan_idx].max_scan_time =
					cpu_to_le16(adapter->active_scan_time);

			if (ch->flags & IEEE80211_CHAN_PASSIVE_SCAN)
				scan_chan_list[chan_idx].chan_scan_mode_bitmap
					|= MWIFIEX_PASSIVE_SCAN;
			else
				scan_chan_list[chan_idx].chan_scan_mode_bitmap
					&= ~MWIFIEX_PASSIVE_SCAN;
			scan_chan_list[chan_idx].chan_number =
							(u32) ch->hw_value;
			if (filtered_scan) {
				scan_chan_list[chan_idx].max_scan_time =
				cpu_to_le16(adapter->specific_scan_time);
				scan_chan_list[chan_idx].chan_scan_mode_bitmap
					|= MWIFIEX_DISABLE_CHAN_FILT;
			}
			chan_idx++;
		}

	}
}

static int
mwifiex_scan_channel_list(struct mwifiex_private *priv,
			  u32 max_chan_per_scan, u8 filtered_scan,
			  struct mwifiex_scan_cmd_config *scan_cfg_out,
			  struct mwifiex_ie_types_chan_list_param_set
			  *chan_tlv_out,
			  struct mwifiex_chan_scan_param_set *scan_chan_list)
{
	int ret = 0;
	struct mwifiex_chan_scan_param_set *tmp_chan_list;
	struct mwifiex_chan_scan_param_set *start_chan;

	u32 tlv_idx;
	u32 total_scan_time;
	u32 done_early;

	if (!scan_cfg_out || !chan_tlv_out || !scan_chan_list) {
		dev_dbg(priv->adapter->dev,
			"info: Scan: Null detect: %p, %p, %p\n",
		       scan_cfg_out, chan_tlv_out, scan_chan_list);
		return -1;
	}

	chan_tlv_out->header.type = cpu_to_le16(TLV_TYPE_CHANLIST);

	tmp_chan_list = scan_chan_list;

	while (tmp_chan_list->chan_number) {

		tlv_idx = 0;
		total_scan_time = 0;
		chan_tlv_out->header.len = 0;
		start_chan = tmp_chan_list;
		done_early = false;

		while (tlv_idx < max_chan_per_scan &&
		       tmp_chan_list->chan_number && !done_early) {

			dev_dbg(priv->adapter->dev,
				"info: Scan: Chan(%3d), Radio(%d),"
				" Mode(%d, %d), Dur(%d)\n",
				tmp_chan_list->chan_number,
				tmp_chan_list->radio_type,
				tmp_chan_list->chan_scan_mode_bitmap
				& MWIFIEX_PASSIVE_SCAN,
				(tmp_chan_list->chan_scan_mode_bitmap
				 & MWIFIEX_DISABLE_CHAN_FILT) >> 1,
				le16_to_cpu(tmp_chan_list->max_scan_time));

			memcpy(chan_tlv_out->chan_scan_param + tlv_idx,
			       tmp_chan_list,
			       sizeof(chan_tlv_out->chan_scan_param));

			chan_tlv_out->header.len =
			cpu_to_le16(le16_to_cpu(chan_tlv_out->header.len) +
			(sizeof(chan_tlv_out->chan_scan_param)));

			scan_cfg_out->tlv_buf_len = (u32) ((u8 *) chan_tlv_out -
							scan_cfg_out->tlv_buf);

			scan_cfg_out->tlv_buf_len +=
				(sizeof(chan_tlv_out->header)
				 + le16_to_cpu(chan_tlv_out->header.len));

			tlv_idx++;

			
			total_scan_time +=
				le16_to_cpu(tmp_chan_list->max_scan_time);

			done_early = false;

			if (!filtered_scan &&
			    (tmp_chan_list->chan_number == 1 ||
			     tmp_chan_list->chan_number == 6 ||
			     tmp_chan_list->chan_number == 11))
				done_early = true;

			tmp_chan_list++;

			if (!filtered_scan &&
			    (tmp_chan_list->chan_number == 1 ||
			     tmp_chan_list->chan_number == 6 ||
			     tmp_chan_list->chan_number == 11))
				done_early = true;
		}

		if (total_scan_time > MWIFIEX_MAX_TOTAL_SCAN_TIME) {
			dev_err(priv->adapter->dev, "total scan time %dms"
				" is over limit (%dms), scan skipped\n",
				total_scan_time, MWIFIEX_MAX_TOTAL_SCAN_TIME);
			ret = -1;
			break;
		}

		priv->adapter->scan_channels = start_chan;

		ret = mwifiex_send_cmd_async(priv, HostCmd_CMD_802_11_SCAN,
					     HostCmd_ACT_GEN_SET, 0,
					     scan_cfg_out);
		if (ret)
			break;
	}

	if (ret)
		return -1;

	return 0;
}

static void
mwifiex_config_scan(struct mwifiex_private *priv,
		    const struct mwifiex_user_scan_cfg *user_scan_in,
		    struct mwifiex_scan_cmd_config *scan_cfg_out,
		    struct mwifiex_ie_types_chan_list_param_set **chan_list_out,
		    struct mwifiex_chan_scan_param_set *scan_chan_list,
		    u8 *max_chan_per_scan, u8 *filtered_scan,
		    u8 *scan_current_only)
{
	struct mwifiex_adapter *adapter = priv->adapter;
	struct mwifiex_ie_types_num_probes *num_probes_tlv;
	struct mwifiex_ie_types_wildcard_ssid_params *wildcard_ssid_tlv;
	struct mwifiex_ie_types_rates_param_set *rates_tlv;
	const u8 zero_mac[ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
	u8 *tlv_pos;
	u32 num_probes;
	u32 ssid_len;
	u32 chan_idx;
	u32 scan_type;
	u16 scan_dur;
	u8 channel;
	u8 radio_type;
	int i;
	u8 ssid_filter;
	u8 rates[MWIFIEX_SUPPORTED_RATES];
	u32 rates_size;
	struct mwifiex_ie_types_htcap *ht_cap;

	scan_cfg_out->tlv_buf_len = 0;

	tlv_pos = scan_cfg_out->tlv_buf;

	*filtered_scan = false;

	*scan_current_only = false;

	if (user_scan_in) {

		ssid_filter = true;

		scan_cfg_out->bss_mode =
			(user_scan_in->bss_mode ? (u8) user_scan_in->
			 bss_mode : (u8) adapter->scan_mode);

		num_probes =
			(user_scan_in->num_probes ? user_scan_in->
			 num_probes : adapter->scan_probes);

		memcpy(scan_cfg_out->specific_bssid,
		       user_scan_in->specific_bssid,
		       sizeof(scan_cfg_out->specific_bssid));

		for (i = 0; i < user_scan_in->num_ssids; i++) {
			ssid_len = user_scan_in->ssid_list[i].ssid_len;

			wildcard_ssid_tlv =
				(struct mwifiex_ie_types_wildcard_ssid_params *)
				tlv_pos;
			wildcard_ssid_tlv->header.type =
				cpu_to_le16(TLV_TYPE_WILDCARDSSID);
			wildcard_ssid_tlv->header.len = cpu_to_le16(
				(u16) (ssid_len + sizeof(wildcard_ssid_tlv->
							 max_ssid_length)));

			if (ssid_len)
				wildcard_ssid_tlv->max_ssid_length = 0;
			else
				wildcard_ssid_tlv->max_ssid_length =
							IEEE80211_MAX_SSID_LEN;

			memcpy(wildcard_ssid_tlv->ssid,
			       user_scan_in->ssid_list[i].ssid, ssid_len);

			tlv_pos += (sizeof(wildcard_ssid_tlv->header)
				+ le16_to_cpu(wildcard_ssid_tlv->header.len));

			dev_dbg(adapter->dev, "info: scan: ssid[%d]: %s, %d\n",
				i, wildcard_ssid_tlv->ssid,
				wildcard_ssid_tlv->max_ssid_length);

			if (!ssid_len && wildcard_ssid_tlv->max_ssid_length)
				ssid_filter = false;
		}

		if ((i && ssid_filter) ||
		    memcmp(scan_cfg_out->specific_bssid, &zero_mac,
			   sizeof(zero_mac)))
			*filtered_scan = true;
	} else {
		scan_cfg_out->bss_mode = (u8) adapter->scan_mode;
		num_probes = adapter->scan_probes;
	}

	if (*filtered_scan)
		*max_chan_per_scan = MWIFIEX_MAX_CHANNELS_PER_SPECIFIC_SCAN;
	else
		*max_chan_per_scan = MWIFIEX_CHANNELS_PER_SCAN_CMD;

	if (num_probes) {

		dev_dbg(adapter->dev, "info: scan: num_probes = %d\n",
			num_probes);

		num_probes_tlv = (struct mwifiex_ie_types_num_probes *) tlv_pos;
		num_probes_tlv->header.type = cpu_to_le16(TLV_TYPE_NUMPROBES);
		num_probes_tlv->header.len =
			cpu_to_le16(sizeof(num_probes_tlv->num_probes));
		num_probes_tlv->num_probes = cpu_to_le16((u16) num_probes);

		tlv_pos += sizeof(num_probes_tlv->header) +
			le16_to_cpu(num_probes_tlv->header.len);

	}

	
	memset(rates, 0, sizeof(rates));

	rates_size = mwifiex_get_supported_rates(priv, rates);

	rates_tlv = (struct mwifiex_ie_types_rates_param_set *) tlv_pos;
	rates_tlv->header.type = cpu_to_le16(WLAN_EID_SUPP_RATES);
	rates_tlv->header.len = cpu_to_le16((u16) rates_size);
	memcpy(rates_tlv->rates, rates, rates_size);
	tlv_pos += sizeof(rates_tlv->header) + rates_size;

	dev_dbg(adapter->dev, "info: SCAN_CMD: Rates size = %d\n", rates_size);

	if (ISSUPP_11NENABLED(priv->adapter->fw_cap_info) &&
	    (priv->adapter->config_bands & BAND_GN ||
	     priv->adapter->config_bands & BAND_AN)) {
		ht_cap = (struct mwifiex_ie_types_htcap *) tlv_pos;
		memset(ht_cap, 0, sizeof(struct mwifiex_ie_types_htcap));
		ht_cap->header.type = cpu_to_le16(WLAN_EID_HT_CAPABILITY);
		ht_cap->header.len =
				cpu_to_le16(sizeof(struct ieee80211_ht_cap));
		radio_type =
			mwifiex_band_to_radio_type(priv->adapter->config_bands);
		mwifiex_fill_cap_info(priv, radio_type, ht_cap);
		tlv_pos += sizeof(struct mwifiex_ie_types_htcap);
	}

	
	mwifiex_cmd_append_vsie_tlv(priv, MWIFIEX_VSIE_MASK_SCAN, &tlv_pos);

	*chan_list_out =
		(struct mwifiex_ie_types_chan_list_param_set *) tlv_pos;

	if (user_scan_in && user_scan_in->chan_list[0].chan_number) {

		dev_dbg(adapter->dev, "info: Scan: Using supplied channel list\n");

		for (chan_idx = 0;
		     chan_idx < MWIFIEX_USER_SCAN_CHAN_MAX &&
		     user_scan_in->chan_list[chan_idx].chan_number;
		     chan_idx++) {

			channel = user_scan_in->chan_list[chan_idx].chan_number;
			(scan_chan_list + chan_idx)->chan_number = channel;

			radio_type =
				user_scan_in->chan_list[chan_idx].radio_type;
			(scan_chan_list + chan_idx)->radio_type = radio_type;

			scan_type = user_scan_in->chan_list[chan_idx].scan_type;

			if (scan_type == MWIFIEX_SCAN_TYPE_PASSIVE)
				(scan_chan_list +
				 chan_idx)->chan_scan_mode_bitmap
					|= MWIFIEX_PASSIVE_SCAN;
			else
				(scan_chan_list +
				 chan_idx)->chan_scan_mode_bitmap
					&= ~MWIFIEX_PASSIVE_SCAN;

			if (user_scan_in->chan_list[chan_idx].scan_time) {
				scan_dur = (u16) user_scan_in->
					chan_list[chan_idx].scan_time;
			} else {
				if (scan_type == MWIFIEX_SCAN_TYPE_PASSIVE)
					scan_dur = adapter->passive_scan_time;
				else if (*filtered_scan)
					scan_dur = adapter->specific_scan_time;
				else
					scan_dur = adapter->active_scan_time;
			}

			(scan_chan_list + chan_idx)->min_scan_time =
				cpu_to_le16(scan_dur);
			(scan_chan_list + chan_idx)->max_scan_time =
				cpu_to_le16(scan_dur);
		}

		
		if ((chan_idx == 1) &&
		    (user_scan_in->chan_list[0].chan_number ==
		     priv->curr_bss_params.bss_descriptor.channel)) {
			*scan_current_only = true;
			dev_dbg(adapter->dev,
				"info: Scan: Scanning current channel only\n");
		}

	} else {
		dev_dbg(adapter->dev,
			"info: Scan: Creating full region channel list\n");
		mwifiex_scan_create_channel_list(priv, user_scan_in,
						 scan_chan_list,
						 *filtered_scan);
	}
}

static void
mwifiex_ret_802_11_scan_get_tlv_ptrs(struct mwifiex_adapter *adapter,
				     struct mwifiex_ie_types_data *tlv,
				     u32 tlv_buf_size, u32 req_tlv_type,
				     struct mwifiex_ie_types_data **tlv_data)
{
	struct mwifiex_ie_types_data *current_tlv;
	u32 tlv_buf_left;
	u32 tlv_type;
	u32 tlv_len;

	current_tlv = tlv;
	tlv_buf_left = tlv_buf_size;
	*tlv_data = NULL;

	dev_dbg(adapter->dev, "info: SCAN_RESP: tlv_buf_size = %d\n",
		tlv_buf_size);

	while (tlv_buf_left >= sizeof(struct mwifiex_ie_types_header)) {

		tlv_type = le16_to_cpu(current_tlv->header.type);
		tlv_len = le16_to_cpu(current_tlv->header.len);

		if (sizeof(tlv->header) + tlv_len > tlv_buf_left) {
			dev_err(adapter->dev, "SCAN_RESP: TLV buffer corrupt\n");
			break;
		}

		if (req_tlv_type == tlv_type) {
			switch (tlv_type) {
			case TLV_TYPE_TSFTIMESTAMP:
				dev_dbg(adapter->dev, "info: SCAN_RESP: TSF "
					"timestamp TLV, len = %d\n", tlv_len);
				*tlv_data = (struct mwifiex_ie_types_data *)
					current_tlv;
				break;
			case TLV_TYPE_CHANNELBANDLIST:
				dev_dbg(adapter->dev, "info: SCAN_RESP: channel"
					" band list TLV, len = %d\n", tlv_len);
				*tlv_data = (struct mwifiex_ie_types_data *)
					current_tlv;
				break;
			default:
				dev_err(adapter->dev,
					"SCAN_RESP: unhandled TLV = %d\n",
				       tlv_type);
				
				return;
			}
		}

		if (*tlv_data)
			break;


		tlv_buf_left -= (sizeof(tlv->header) + tlv_len);
		current_tlv =
			(struct mwifiex_ie_types_data *) (current_tlv->data +
							  tlv_len);

	}			
}

int
mwifiex_update_bss_desc_with_ie(struct mwifiex_adapter *adapter,
				struct mwifiex_bssdescriptor *bss_entry,
				u8 *ie_buf, u32 ie_len)
{
	int ret = 0;
	u8 element_id;
	struct ieee_types_fh_param_set *fh_param_set;
	struct ieee_types_ds_param_set *ds_param_set;
	struct ieee_types_cf_param_set *cf_param_set;
	struct ieee_types_ibss_param_set *ibss_param_set;
	u8 *current_ptr;
	u8 *rate;
	u8 element_len;
	u16 total_ie_len;
	u8 bytes_to_copy;
	u8 rate_size;
	u8 found_data_rate_ie;
	u32 bytes_left;
	struct ieee_types_vendor_specific *vendor_ie;
	const u8 wpa_oui[4] = { 0x00, 0x50, 0xf2, 0x01 };
	const u8 wmm_oui[4] = { 0x00, 0x50, 0xf2, 0x02 };

	found_data_rate_ie = false;
	rate_size = 0;
	current_ptr = ie_buf;
	bytes_left = ie_len;
	bss_entry->beacon_buf = ie_buf;
	bss_entry->beacon_buf_size = ie_len;

	
	while (bytes_left >= 2) {
		element_id = *current_ptr;
		element_len = *(current_ptr + 1);
		total_ie_len = element_len + sizeof(struct ieee_types_header);

		if (bytes_left < total_ie_len) {
			dev_err(adapter->dev, "err: InterpretIE: in processing"
				" IE, bytes left < IE length\n");
			return -1;
		}
		switch (element_id) {
		case WLAN_EID_SSID:
			bss_entry->ssid.ssid_len = element_len;
			memcpy(bss_entry->ssid.ssid, (current_ptr + 2),
			       element_len);
			dev_dbg(adapter->dev,
				"info: InterpretIE: ssid: %-32s\n",
				bss_entry->ssid.ssid);
			break;

		case WLAN_EID_SUPP_RATES:
			memcpy(bss_entry->data_rates, current_ptr + 2,
			       element_len);
			memcpy(bss_entry->supported_rates, current_ptr + 2,
			       element_len);
			rate_size = element_len;
			found_data_rate_ie = true;
			break;

		case WLAN_EID_FH_PARAMS:
			fh_param_set =
				(struct ieee_types_fh_param_set *) current_ptr;
			memcpy(&bss_entry->phy_param_set.fh_param_set,
			       fh_param_set,
			       sizeof(struct ieee_types_fh_param_set));
			break;

		case WLAN_EID_DS_PARAMS:
			ds_param_set =
				(struct ieee_types_ds_param_set *) current_ptr;

			bss_entry->channel = ds_param_set->current_chan;

			memcpy(&bss_entry->phy_param_set.ds_param_set,
			       ds_param_set,
			       sizeof(struct ieee_types_ds_param_set));
			break;

		case WLAN_EID_CF_PARAMS:
			cf_param_set =
				(struct ieee_types_cf_param_set *) current_ptr;
			memcpy(&bss_entry->ss_param_set.cf_param_set,
			       cf_param_set,
			       sizeof(struct ieee_types_cf_param_set));
			break;

		case WLAN_EID_IBSS_PARAMS:
			ibss_param_set =
				(struct ieee_types_ibss_param_set *)
				current_ptr;
			memcpy(&bss_entry->ss_param_set.ibss_param_set,
			       ibss_param_set,
			       sizeof(struct ieee_types_ibss_param_set));
			break;

		case WLAN_EID_ERP_INFO:
			bss_entry->erp_flags = *(current_ptr + 2);
			break;

		case WLAN_EID_EXT_SUPP_RATES:
			if (found_data_rate_ie) {
				if ((element_len + rate_size) >
				    MWIFIEX_SUPPORTED_RATES)
					bytes_to_copy =
						(MWIFIEX_SUPPORTED_RATES -
						 rate_size);
				else
					bytes_to_copy = element_len;

				rate = (u8 *) bss_entry->data_rates;
				rate += rate_size;
				memcpy(rate, current_ptr + 2, bytes_to_copy);

				rate = (u8 *) bss_entry->supported_rates;
				rate += rate_size;
				memcpy(rate, current_ptr + 2, bytes_to_copy);
			}
			break;

		case WLAN_EID_VENDOR_SPECIFIC:
			vendor_ie = (struct ieee_types_vendor_specific *)
					current_ptr;

			if (!memcmp
			    (vendor_ie->vend_hdr.oui, wpa_oui,
			     sizeof(wpa_oui))) {
				bss_entry->bcn_wpa_ie =
					(struct ieee_types_vendor_specific *)
					current_ptr;
				bss_entry->wpa_offset = (u16)
					(current_ptr - bss_entry->beacon_buf);
			} else if (!memcmp(vendor_ie->vend_hdr.oui, wmm_oui,
				    sizeof(wmm_oui))) {
				if (total_ie_len ==
				    sizeof(struct ieee_types_wmm_parameter) ||
				    total_ie_len ==
				    sizeof(struct ieee_types_wmm_info))
					memcpy((u8 *) &bss_entry->wmm_ie,
					       current_ptr, total_ie_len);
			}
			break;
		case WLAN_EID_RSN:
			bss_entry->bcn_rsn_ie =
				(struct ieee_types_generic *) current_ptr;
			bss_entry->rsn_offset = (u16) (current_ptr -
							bss_entry->beacon_buf);
			break;
		case WLAN_EID_BSS_AC_ACCESS_DELAY:
			bss_entry->bcn_wapi_ie =
				(struct ieee_types_generic *) current_ptr;
			bss_entry->wapi_offset = (u16) (current_ptr -
							bss_entry->beacon_buf);
			break;
		case WLAN_EID_HT_CAPABILITY:
			bss_entry->bcn_ht_cap = (struct ieee80211_ht_cap *)
					(current_ptr +
					sizeof(struct ieee_types_header));
			bss_entry->ht_cap_offset = (u16) (current_ptr +
					sizeof(struct ieee_types_header) -
					bss_entry->beacon_buf);
			break;
		case WLAN_EID_HT_INFORMATION:
			bss_entry->bcn_ht_info = (struct ieee80211_ht_info *)
					(current_ptr +
					sizeof(struct ieee_types_header));
			bss_entry->ht_info_offset = (u16) (current_ptr +
					sizeof(struct ieee_types_header) -
					bss_entry->beacon_buf);
			break;
		case WLAN_EID_BSS_COEX_2040:
			bss_entry->bcn_bss_co_2040 = (u8 *) (current_ptr +
					sizeof(struct ieee_types_header));
			bss_entry->bss_co_2040_offset = (u16) (current_ptr +
					sizeof(struct ieee_types_header) -
						bss_entry->beacon_buf);
			break;
		case WLAN_EID_EXT_CAPABILITY:
			bss_entry->bcn_ext_cap = (u8 *) (current_ptr +
					sizeof(struct ieee_types_header));
			bss_entry->ext_cap_offset = (u16) (current_ptr +
					sizeof(struct ieee_types_header) -
					bss_entry->beacon_buf);
			break;
		default:
			break;
		}

		current_ptr += element_len + 2;

		
		bytes_left -= (element_len + 2);

	}	
	return ret;
}

static u8
mwifiex_radio_type_to_band(u8 radio_type)
{
	switch (radio_type) {
	case HostCmd_SCAN_RADIO_TYPE_A:
		return BAND_A;
	case HostCmd_SCAN_RADIO_TYPE_BG:
	default:
		return BAND_G;
	}
}

static int mwifiex_scan_networks(struct mwifiex_private *priv,
		const struct mwifiex_user_scan_cfg *user_scan_in)
{
	int ret = 0;
	struct mwifiex_adapter *adapter = priv->adapter;
	struct cmd_ctrl_node *cmd_node;
	union mwifiex_scan_cmd_config_tlv *scan_cfg_out;
	struct mwifiex_ie_types_chan_list_param_set *chan_list_out;
	u32 buf_size;
	struct mwifiex_chan_scan_param_set *scan_chan_list;
	u8 filtered_scan;
	u8 scan_current_chan_only;
	u8 max_chan_per_scan;
	unsigned long flags;

	if (adapter->scan_processing) {
		dev_dbg(adapter->dev, "cmd: Scan already in process...\n");
		return ret;
	}

	spin_lock_irqsave(&adapter->mwifiex_cmd_lock, flags);
	adapter->scan_processing = true;
	spin_unlock_irqrestore(&adapter->mwifiex_cmd_lock, flags);

	if (priv->scan_block) {
		dev_dbg(adapter->dev,
			"cmd: Scan is blocked during association...\n");
		return ret;
	}

	scan_cfg_out = kzalloc(sizeof(union mwifiex_scan_cmd_config_tlv),
								GFP_KERNEL);
	if (!scan_cfg_out) {
		dev_err(adapter->dev, "failed to alloc scan_cfg_out\n");
		return -ENOMEM;
	}

	buf_size = sizeof(struct mwifiex_chan_scan_param_set) *
						MWIFIEX_USER_SCAN_CHAN_MAX;
	scan_chan_list = kzalloc(buf_size, GFP_KERNEL);
	if (!scan_chan_list) {
		dev_err(adapter->dev, "failed to alloc scan_chan_list\n");
		kfree(scan_cfg_out);
		return -ENOMEM;
	}

	mwifiex_config_scan(priv, user_scan_in, &scan_cfg_out->config,
			    &chan_list_out, scan_chan_list, &max_chan_per_scan,
			    &filtered_scan, &scan_current_chan_only);

	ret = mwifiex_scan_channel_list(priv, max_chan_per_scan, filtered_scan,
					&scan_cfg_out->config, chan_list_out,
					scan_chan_list);

	
	if (!ret) {
		spin_lock_irqsave(&adapter->scan_pending_q_lock, flags);
		if (!list_empty(&adapter->scan_pending_q)) {
			cmd_node = list_first_entry(&adapter->scan_pending_q,
						    struct cmd_ctrl_node, list);
			list_del(&cmd_node->list);
			spin_unlock_irqrestore(&adapter->scan_pending_q_lock,
					       flags);
			adapter->cmd_queued = cmd_node;
			mwifiex_insert_cmd_to_pending_q(adapter, cmd_node,
							true);
		} else {
			spin_unlock_irqrestore(&adapter->scan_pending_q_lock,
					       flags);
		}
	} else {
		spin_lock_irqsave(&adapter->mwifiex_cmd_lock, flags);
		adapter->scan_processing = true;
		spin_unlock_irqrestore(&adapter->mwifiex_cmd_lock, flags);
	}

	kfree(scan_cfg_out);
	kfree(scan_chan_list);
	return ret;
}

int mwifiex_set_user_scan_ioctl(struct mwifiex_private *priv,
				struct mwifiex_user_scan_cfg *scan_req)
{
	int status;

	status = mwifiex_scan_networks(priv, scan_req);
	queue_work(priv->adapter->workqueue, &priv->adapter->main_work);

	return status;
}

int mwifiex_cmd_802_11_scan(struct host_cmd_ds_command *cmd,
			    struct mwifiex_scan_cmd_config *scan_cfg)
{
	struct host_cmd_ds_802_11_scan *scan_cmd = &cmd->params.scan;

	
	scan_cmd->bss_mode = scan_cfg->bss_mode;
	memcpy(scan_cmd->bssid, scan_cfg->specific_bssid,
	       sizeof(scan_cmd->bssid));
	memcpy(scan_cmd->tlv_buffer, scan_cfg->tlv_buf, scan_cfg->tlv_buf_len);

	cmd->command = cpu_to_le16(HostCmd_CMD_802_11_SCAN);

	
	cmd->size = cpu_to_le16((u16) (sizeof(scan_cmd->bss_mode)
					  + sizeof(scan_cmd->bssid)
					  + scan_cfg->tlv_buf_len + S_DS_GEN));

	return 0;
}

int mwifiex_check_network_compatibility(struct mwifiex_private *priv,
					struct mwifiex_bssdescriptor *bss_desc)
{
	int ret = -1;

	if (!bss_desc)
		return -1;

	if ((mwifiex_get_cfp(priv, (u8) bss_desc->bss_band,
			     (u16) bss_desc->channel, 0))) {
		switch (priv->bss_mode) {
		case NL80211_IFTYPE_STATION:
		case NL80211_IFTYPE_ADHOC:
			ret = mwifiex_is_network_compatible(priv, bss_desc,
							    priv->bss_mode);
			if (ret)
				dev_err(priv->adapter->dev, "cannot find ssid "
					"%s\n", bss_desc->ssid.ssid);
				break;
		default:
				ret = 0;
		}
	}

	return ret;
}

static int
mwifiex_update_curr_bss_params(struct mwifiex_private *priv, u8 *bssid,
			       s32 rssi, const u8 *ie_buf, size_t ie_len,
			       u16 beacon_period, u16 cap_info_bitmap, u8 band)
{
	struct mwifiex_bssdescriptor *bss_desc;
	int ret;
	unsigned long flags;
	u8 *beacon_ie;

	
	bss_desc = kzalloc(sizeof(struct mwifiex_bssdescriptor),
			GFP_KERNEL);
	if (!bss_desc) {
		dev_err(priv->adapter->dev, " failed to alloc bss_desc\n");
		return -ENOMEM;
	}

	beacon_ie = kmemdup(ie_buf, ie_len, GFP_KERNEL);
	if (!beacon_ie) {
		kfree(bss_desc);
		dev_err(priv->adapter->dev, " failed to alloc beacon_ie\n");
		return -ENOMEM;
	}

	ret = mwifiex_fill_new_bss_desc(priv, bssid, rssi, beacon_ie,
					ie_len, beacon_period,
					cap_info_bitmap, band, bss_desc);
	if (ret)
		goto done;

	ret = mwifiex_check_network_compatibility(priv, bss_desc);
	if (ret)
		goto done;

	
	spin_lock_irqsave(&priv->curr_bcn_buf_lock, flags);
	priv->curr_bss_params.bss_descriptor.bcn_wpa_ie = NULL;
	priv->curr_bss_params.bss_descriptor.wpa_offset = 0;
	priv->curr_bss_params.bss_descriptor.bcn_rsn_ie = NULL;
	priv->curr_bss_params.bss_descriptor.rsn_offset = 0;
	priv->curr_bss_params.bss_descriptor.bcn_wapi_ie = NULL;
	priv->curr_bss_params.bss_descriptor.wapi_offset = 0;
	priv->curr_bss_params.bss_descriptor.bcn_ht_cap = NULL;
	priv->curr_bss_params.bss_descriptor.ht_cap_offset =
		0;
	priv->curr_bss_params.bss_descriptor.bcn_ht_info = NULL;
	priv->curr_bss_params.bss_descriptor.ht_info_offset =
		0;
	priv->curr_bss_params.bss_descriptor.bcn_bss_co_2040 =
		NULL;
	priv->curr_bss_params.bss_descriptor.
		bss_co_2040_offset = 0;
	priv->curr_bss_params.bss_descriptor.bcn_ext_cap = NULL;
	priv->curr_bss_params.bss_descriptor.ext_cap_offset = 0;
	priv->curr_bss_params.bss_descriptor.beacon_buf = NULL;
	priv->curr_bss_params.bss_descriptor.beacon_buf_size =
		0;

	
	memcpy(&priv->curr_bss_params.bss_descriptor, bss_desc,
	       sizeof(priv->curr_bss_params.bss_descriptor));
	mwifiex_save_curr_bcn(priv);
	spin_unlock_irqrestore(&priv->curr_bcn_buf_lock, flags);

done:
	kfree(bss_desc);
	kfree(beacon_ie);
	return 0;
}

int mwifiex_ret_802_11_scan(struct mwifiex_private *priv,
			    struct host_cmd_ds_command *resp)
{
	int ret = 0;
	struct mwifiex_adapter *adapter = priv->adapter;
	struct cmd_ctrl_node *cmd_node;
	struct host_cmd_ds_802_11_scan_rsp *scan_rsp;
	struct mwifiex_ie_types_data *tlv_data;
	struct mwifiex_ie_types_tsf_timestamp *tsf_tlv;
	u8 *bss_info;
	u32 scan_resp_size;
	u32 bytes_left;
	u32 idx;
	u32 tlv_buf_size;
	struct mwifiex_chan_freq_power *cfp;
	struct mwifiex_ie_types_chan_band_list_param_set *chan_band_tlv;
	struct chan_band_param_set *chan_band;
	u8 is_bgscan_resp;
	unsigned long flags;
	struct cfg80211_bss *bss;

	is_bgscan_resp = (le16_to_cpu(resp->command)
			  == HostCmd_CMD_802_11_BG_SCAN_QUERY);
	if (is_bgscan_resp)
		scan_rsp = &resp->params.bg_scan_query_resp.scan_resp;
	else
		scan_rsp = &resp->params.scan_resp;


	if (scan_rsp->number_of_sets > MWIFIEX_MAX_AP) {
		dev_err(adapter->dev, "SCAN_RESP: too many AP returned (%d)\n",
			scan_rsp->number_of_sets);
		ret = -1;
		goto done;
	}

	bytes_left = le16_to_cpu(scan_rsp->bss_descript_size);
	dev_dbg(adapter->dev, "info: SCAN_RESP: bss_descript_size %d\n",
		bytes_left);

	scan_resp_size = le16_to_cpu(resp->size);

	dev_dbg(adapter->dev,
		"info: SCAN_RESP: returned %d APs before parsing\n",
		scan_rsp->number_of_sets);

	bss_info = scan_rsp->bss_desc_and_tlv_buffer;

	tlv_buf_size = scan_resp_size - (bytes_left
					 + sizeof(scan_rsp->bss_descript_size)
					 + sizeof(scan_rsp->number_of_sets)
					 + S_DS_GEN);

	tlv_data = (struct mwifiex_ie_types_data *) (scan_rsp->
						 bss_desc_and_tlv_buffer +
						 bytes_left);

	mwifiex_ret_802_11_scan_get_tlv_ptrs(adapter, tlv_data, tlv_buf_size,
					     TLV_TYPE_TSFTIMESTAMP,
					     (struct mwifiex_ie_types_data **)
					     &tsf_tlv);

	mwifiex_ret_802_11_scan_get_tlv_ptrs(adapter, tlv_data, tlv_buf_size,
					     TLV_TYPE_CHANNELBANDLIST,
					     (struct mwifiex_ie_types_data **)
					     &chan_band_tlv);

	for (idx = 0; idx < scan_rsp->number_of_sets && bytes_left; idx++) {
		u8 bssid[ETH_ALEN];
		s32 rssi;
		const u8 *ie_buf;
		size_t ie_len;
		u16 channel = 0;
		u64 network_tsf = 0;
		u16 beacon_size = 0;
		u32 curr_bcn_bytes;
		u32 freq;
		u16 beacon_period;
		u16 cap_info_bitmap;
		u8 *current_ptr;
		struct mwifiex_bcn_param *bcn_param;

		if (bytes_left >= sizeof(beacon_size)) {
			
			memcpy(&beacon_size, bss_info, sizeof(beacon_size));
			bytes_left -= sizeof(beacon_size);
			bss_info += sizeof(beacon_size);
		}

		if (!beacon_size || beacon_size > bytes_left) {
			bss_info += bytes_left;
			bytes_left = 0;
			return -1;
		}

		current_ptr = bss_info;

		
		bss_info += beacon_size;
		bytes_left -= beacon_size;

		curr_bcn_bytes = beacon_size;

		if (curr_bcn_bytes < sizeof(struct mwifiex_bcn_param)) {
			dev_err(adapter->dev,
				"InterpretIE: not enough bytes left\n");
			continue;
		}
		bcn_param = (struct mwifiex_bcn_param *)current_ptr;
		current_ptr += sizeof(*bcn_param);
		curr_bcn_bytes -= sizeof(*bcn_param);

		memcpy(bssid, bcn_param->bssid, ETH_ALEN);

		rssi = (s32) (bcn_param->rssi);
		dev_dbg(adapter->dev, "info: InterpretIE: RSSI=%02X\n", rssi);

		beacon_period = le16_to_cpu(bcn_param->beacon_period);

		cap_info_bitmap = le16_to_cpu(bcn_param->cap_info_bitmap);
		dev_dbg(adapter->dev, "info: InterpretIE: capabilities=0x%X\n",
			cap_info_bitmap);

		
		ie_buf = current_ptr;
		ie_len = curr_bcn_bytes;
		dev_dbg(adapter->dev,
			"info: InterpretIE: IELength for this AP = %d\n",
			curr_bcn_bytes);

		while (curr_bcn_bytes >= sizeof(struct ieee_types_header)) {
			u8 element_id, element_len;

			element_id = *current_ptr;
			element_len = *(current_ptr + 1);
			if (curr_bcn_bytes < element_len +
					sizeof(struct ieee_types_header)) {
				dev_err(priv->adapter->dev,
					"%s: bytes left < IE length\n",
					__func__);
				goto done;
			}
			if (element_id == WLAN_EID_DS_PARAMS) {
				channel = *(u8 *) (current_ptr +
					sizeof(struct ieee_types_header));
				break;
			}

			current_ptr += element_len +
					sizeof(struct ieee_types_header);
			curr_bcn_bytes -= element_len +
					sizeof(struct ieee_types_header);
		}

		if (tsf_tlv)
			memcpy(&network_tsf,
			       &tsf_tlv->tsf_data[idx * TSF_DATA_SIZE],
			       sizeof(network_tsf));

		if (channel) {
			struct ieee80211_channel *chan;
			u8 band;

			band = BAND_G;
			if (chan_band_tlv) {
				chan_band =
					&chan_band_tlv->chan_band_param[idx];
				band = mwifiex_radio_type_to_band(
						chan_band->radio_type
						& (BIT(0) | BIT(1)));
			}

			cfp = mwifiex_get_cfp(priv, band, channel, 0);

			freq = cfp ? cfp->freq : 0;

			chan = ieee80211_get_channel(priv->wdev->wiphy, freq);

			if (chan && !(chan->flags & IEEE80211_CHAN_DISABLED)) {
				bss = cfg80211_inform_bss(priv->wdev->wiphy,
					      chan, bssid, network_tsf,
					      cap_info_bitmap, beacon_period,
					      ie_buf, ie_len, rssi, GFP_KERNEL);
				*(u8 *)bss->priv = band;
				cfg80211_put_bss(bss);

				if (priv->media_connected &&
				    !memcmp(bssid,
					    priv->curr_bss_params.bss_descriptor
					    .mac_address, ETH_ALEN))
					mwifiex_update_curr_bss_params
							(priv, bssid, rssi,
							 ie_buf, ie_len,
							 beacon_period,
							 cap_info_bitmap, band);
			}
		} else {
			dev_dbg(adapter->dev, "missing BSS channel IE\n");
		}
	}

	spin_lock_irqsave(&adapter->scan_pending_q_lock, flags);
	if (list_empty(&adapter->scan_pending_q)) {
		spin_unlock_irqrestore(&adapter->scan_pending_q_lock, flags);
		spin_lock_irqsave(&adapter->mwifiex_cmd_lock, flags);
		adapter->scan_processing = false;
		spin_unlock_irqrestore(&adapter->mwifiex_cmd_lock, flags);

		
		if (adapter->curr_cmd->wait_q_enabled) {
			adapter->cmd_wait_q.status = 0;
			mwifiex_complete_cmd(adapter, adapter->curr_cmd);
		}
		if (priv->report_scan_result)
			priv->report_scan_result = false;
		if (priv->scan_pending_on_block) {
			priv->scan_pending_on_block = false;
			up(&priv->async_sem);
		}

		if (priv->user_scan_cfg) {
			dev_dbg(priv->adapter->dev,
				"info: %s: sending scan results\n", __func__);
			cfg80211_scan_done(priv->scan_request, 0);
			priv->scan_request = NULL;
			kfree(priv->user_scan_cfg);
			priv->user_scan_cfg = NULL;
		}
	} else {
		cmd_node = list_first_entry(&adapter->scan_pending_q,
					    struct cmd_ctrl_node, list);
		list_del(&cmd_node->list);
		spin_unlock_irqrestore(&adapter->scan_pending_q_lock, flags);

		mwifiex_insert_cmd_to_pending_q(adapter, cmd_node, true);
	}

done:
	return ret;
}

int mwifiex_cmd_802_11_bg_scan_query(struct host_cmd_ds_command *cmd)
{
	struct host_cmd_ds_802_11_bg_scan_query *bg_query =
		&cmd->params.bg_scan_query;

	cmd->command = cpu_to_le16(HostCmd_CMD_802_11_BG_SCAN_QUERY);
	cmd->size = cpu_to_le16(sizeof(struct host_cmd_ds_802_11_bg_scan_query)
				+ S_DS_GEN);

	bg_query->flush = 1;

	return 0;
}

void
mwifiex_queue_scan_cmd(struct mwifiex_private *priv,
		       struct cmd_ctrl_node *cmd_node)
{
	struct mwifiex_adapter *adapter = priv->adapter;
	unsigned long flags;

	cmd_node->wait_q_enabled = true;
	cmd_node->condition = &adapter->scan_wait_q_woken;
	spin_lock_irqsave(&adapter->scan_pending_q_lock, flags);
	list_add_tail(&cmd_node->list, &adapter->scan_pending_q);
	spin_unlock_irqrestore(&adapter->scan_pending_q_lock, flags);
}

static int mwifiex_scan_specific_ssid(struct mwifiex_private *priv,
				      struct cfg80211_ssid *req_ssid)
{
	struct mwifiex_adapter *adapter = priv->adapter;
	int ret = 0;
	struct mwifiex_user_scan_cfg *scan_cfg;

	if (!req_ssid)
		return -1;

	if (adapter->scan_processing) {
		dev_dbg(adapter->dev, "cmd: Scan already in process...\n");
		return ret;
	}

	if (priv->scan_block) {
		dev_dbg(adapter->dev,
			"cmd: Scan is blocked during association...\n");
		return ret;
	}

	scan_cfg = kzalloc(sizeof(struct mwifiex_user_scan_cfg), GFP_KERNEL);
	if (!scan_cfg) {
		dev_err(adapter->dev, "failed to alloc scan_cfg\n");
		return -ENOMEM;
	}

	scan_cfg->ssid_list = req_ssid;
	scan_cfg->num_ssids = 1;

	ret = mwifiex_scan_networks(priv, scan_cfg);

	kfree(scan_cfg);
	return ret;
}

int mwifiex_request_scan(struct mwifiex_private *priv,
			 struct cfg80211_ssid *req_ssid)
{
	int ret;

	if (down_interruptible(&priv->async_sem)) {
		dev_err(priv->adapter->dev, "%s: acquire semaphore\n",
			__func__);
		return -1;
	}
	priv->scan_pending_on_block = true;

	priv->adapter->scan_wait_q_woken = false;

	if (req_ssid && req_ssid->ssid_len != 0)
		
		ret = mwifiex_scan_specific_ssid(priv, req_ssid);
	else
		
		ret = mwifiex_scan_networks(priv, NULL);

	if (!ret)
		ret = mwifiex_wait_queue_complete(priv->adapter);

	if (ret == -1) {
		priv->scan_pending_on_block = false;
		up(&priv->async_sem);
	}

	return ret;
}

int
mwifiex_cmd_append_vsie_tlv(struct mwifiex_private *priv,
			    u16 vsie_mask, u8 **buffer)
{
	int id, ret_len = 0;
	struct mwifiex_ie_types_vendor_param_set *vs_param_set;

	if (!buffer)
		return 0;
	if (!(*buffer))
		return 0;

	for (id = 0; id < MWIFIEX_MAX_VSIE_NUM; id++) {
		if (priv->vs_ie[id].mask & vsie_mask) {
			vs_param_set =
				(struct mwifiex_ie_types_vendor_param_set *)
				*buffer;
			vs_param_set->header.type =
				cpu_to_le16(TLV_TYPE_PASSTHROUGH);
			vs_param_set->header.len =
				cpu_to_le16((((u16) priv->vs_ie[id].ie[1])
				& 0x00FF) + 2);
			memcpy(vs_param_set->ie, priv->vs_ie[id].ie,
			       le16_to_cpu(vs_param_set->header.len));
			*buffer += le16_to_cpu(vs_param_set->header.len) +
				   sizeof(struct mwifiex_ie_types_header);
			ret_len += le16_to_cpu(vs_param_set->header.len) +
				   sizeof(struct mwifiex_ie_types_header);
		}
	}
	return ret_len;
}

void
mwifiex_save_curr_bcn(struct mwifiex_private *priv)
{
	struct mwifiex_bssdescriptor *curr_bss =
		&priv->curr_bss_params.bss_descriptor;

	if (!curr_bss->beacon_buf_size)
		return;

	
	if (!priv->curr_bcn_buf ||
	    priv->curr_bcn_size != curr_bss->beacon_buf_size) {
		priv->curr_bcn_size = curr_bss->beacon_buf_size;

		kfree(priv->curr_bcn_buf);
		priv->curr_bcn_buf = kmalloc(curr_bss->beacon_buf_size,
					     GFP_ATOMIC);
		if (!priv->curr_bcn_buf) {
			dev_err(priv->adapter->dev,
				"failed to alloc curr_bcn_buf\n");
			return;
		}
	}

	memcpy(priv->curr_bcn_buf, curr_bss->beacon_buf,
	       curr_bss->beacon_buf_size);
	dev_dbg(priv->adapter->dev, "info: current beacon saved %d\n",
		priv->curr_bcn_size);

	curr_bss->beacon_buf = priv->curr_bcn_buf;

	
	if (curr_bss->bcn_wpa_ie)
		curr_bss->bcn_wpa_ie =
			(struct ieee_types_vendor_specific *)
			(curr_bss->beacon_buf +
			 curr_bss->wpa_offset);

	if (curr_bss->bcn_rsn_ie)
		curr_bss->bcn_rsn_ie = (struct ieee_types_generic *)
			(curr_bss->beacon_buf +
			 curr_bss->rsn_offset);

	if (curr_bss->bcn_ht_cap)
		curr_bss->bcn_ht_cap = (struct ieee80211_ht_cap *)
			(curr_bss->beacon_buf +
			 curr_bss->ht_cap_offset);

	if (curr_bss->bcn_ht_info)
		curr_bss->bcn_ht_info = (struct ieee80211_ht_info *)
			(curr_bss->beacon_buf +
			 curr_bss->ht_info_offset);

	if (curr_bss->bcn_bss_co_2040)
		curr_bss->bcn_bss_co_2040 =
			(u8 *) (curr_bss->beacon_buf +
					curr_bss->bss_co_2040_offset);

	if (curr_bss->bcn_ext_cap)
		curr_bss->bcn_ext_cap = (u8 *) (curr_bss->beacon_buf +
				curr_bss->ext_cap_offset);
}

void
mwifiex_free_curr_bcn(struct mwifiex_private *priv)
{
	kfree(priv->curr_bcn_buf);
	priv->curr_bcn_buf = NULL;
}
