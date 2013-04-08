<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta HTTP-EQUIV="Pragma" CONTENT="no-cache">
<meta HTTP-EQUIV="Expires" CONTENT="-1">
<title>Wireless Router <#Web_Title#> - <#menu5_4_4#></title>
<link rel="stylesheet" type="text/css" href="/bootstrap/css/bootstrap.css">
<link rel="stylesheet" type="text/css" href="/bootstrap/css/main.css">
<link rel="stylesheet" type="text/css" href="/bootstrap/css/engage.itoggle.css">

<script type="text/javascript" src="/jquery.js"></script>
<script type="text/javascript" src="/bootstrap/js/bootstrap.min.js"></script>
<script type="text/javascript" src="/bootstrap/js/engage.itoggle.min.js"></script>
<script type="text/javascript" src="/state.js"></script>
<script type="text/javascript" src="/general.js"></script>
<script type="text/javascript" src="/popup.js"></script>
<script type="text/javascript" src="/help.js"></script>
<script type="text/javascript" src="/detect.js"></script>
<script type="text/javascript" src="/wcdma_list.js"></script>
<script type="text/javascript" src="/cdma2000_list.js"></script>
<script type="text/javascript" src="/td-scdma_list.js"></script>
<script type="text/javascript" src="/ndis_list.js"></script>

<script>
    var $j = jQuery.noConflict();
    $j(document).ready(function() {
        $j('#modem_rule_on_of').iToggle({
            easing: 'linear',
            speed: 70,
            onClickOn: function(){
                $j("#modem_rule_fake").attr("checked", "checked").attr("value", 1);
                $j("#modem_rule_1").attr("checked", "checked");
                $j("#modem_rule_0").removeAttr("checked");
                switch_modem_rule();
            },
            onClickOff: function(){
                $j("#modem_rule_fake").removeAttr("checked").attr("value", 0);
                $j("#modem_rule_0").attr("checked", "checked");
                $j("#modem_rule_1").removeAttr("checked");
                switch_modem_rule();
            }
        });
        $j("#modem_rule_on_of label.itoggle").css("background-position", $j("input#modem_rule_fake:checked").length > 0 ? '0% -27px' : '100% -27px');
    });

</script>

<script>

<% login_state_hook(); %>

var country = '<% nvram_get_x("General", "modem_country"); %>';
var isp = '<% nvram_get_x("General", "modem_isp"); %>';
var apn = '<% nvram_get_x("General", "modem_apn"); %>';
var dialnum = '<% nvram_get_x("General", "modem_dialnum"); %>';
var user = '<% nvram_get_x("General", "modem_user"); %>';
var pass = '<% nvram_get_x("General", "modem_pass"); %>';

var countrylist = new Array();
var isplist = new Array();
var apnlist = new Array();
var daillist = new Array();
var userlist = new Array();
var passlist = new Array();

function initial(){
	show_banner(1);
	show_menu(5, 7, 4);
	show_footer();

	switch_modem_type();
	enable_auto_hint(21, 7);
}

function switch_modem_rule(){
	var mrule = document.form.modem_rule[0].checked;
	var mtype = document.form.modem_type.value;

	if (!mrule){
		$("row_modem_type").style.display = "none";
		$("row_modem_country").style.display = "none";
		$("row_modem_isp").style.display = "none";
		$("row_modem_arun").style.display = "none";
	}
	else {
		$("row_modem_type").style.display = "";
		$("row_modem_country").style.display = "";
		$("row_modem_isp").style.display = "";
		$("row_modem_arun").style.display = "";
	}

	if (mtype == "3" || !mrule) {
		if (!mrule) {
			$("row_modem_ras_1").style.display = "none";
			$("row_modem_ras_6").style.display = "none";
			$("row_modem_ras_7").style.display = "none";
			$("row_modem_ras_9").style.display = "none";
		}
		else {
			$("row_modem_ras_1").style.display = "";
			$("row_modem_ras_6").style.display = "";
			$("row_modem_ras_7").style.display = "";
			$("row_modem_ras_9").style.display = "";
		}
		$("row_modem_ras_2").style.display = "none";
		$("row_modem_ras_3").style.display = "none";
		$("row_modem_ras_4").style.display = "none";
		$("row_modem_ras_5").style.display = "none";
		$("row_modem_ras_8").style.display = "none";
	}
	else {
		if (mtype == "1") {
			$("row_modem_ras_1").style.display = "none";
			$("row_modem_ras_2").style.display = "none";
		}
		else {
			$("row_modem_ras_1").style.display = "";
			$("row_modem_ras_2").style.display = "";
		}
		
		$("row_modem_ras_3").style.display = "";
		$("row_modem_ras_4").style.display = "";
		$("row_modem_ras_5").style.display = "";
		$("row_modem_ras_6").style.display = "";
		$("row_modem_ras_7").style.display = "";
		$("row_modem_ras_8").style.display = "";
		$("row_modem_ras_9").style.display = "none";
	}
}

function switch_modem_type(){
	var mtype = document.form.modem_type.value;

	switch_modem_rule();

	if (mtype == "3"){
		show_4G_country_list();
	}
	else if (mtype == "2"){
		show_tdscdma_country_list();
	}
	else if (mtype == "1"){
		show_cdma2000_country_list();
	}
	else{
		show_wcdma_country_list();
	}

	gen_list(mtype);
	show_APN_list();
}

function gen_list(mtype){
	if (mtype == "3"){
		gen_4G_list();
	}
	else if (mtype == "2"){
		gen_tdscdma_list();
	}
	else if (mtype == "1"){
		gen_cdma2000_list();
	}
	else{
		gen_wcdma_list();
	}

	free_options($("modem_isp"));
	$("modem_isp").options.length = isplist.length;

	for(var i = 0; i < isplist.length; i++){
		$("modem_isp").options[i] = new Option(isplist[i], isplist[i]);
		if(isplist[i] == isp)
			$("modem_isp").options[i].selected = "1";
	}
}

function show_APN_list(){
	if(document.form.modem_type.value != "3"){
		var ISPlist = $("modem_isp").value;
		if((ISPlist == isp) && (apn != "" || dialnum != "" || user != "" || pass != "")){
			$("modem_apn").value = apn;
			$("modem_dialnum").value = dialnum;
			$("modem_user").value = user;
			$("modem_pass").value = pass;
		}
		else{
			for(var i = 0; i < isplist.length; i++){
				if(isplist[i] == ISPlist){
					$("modem_apn").value = apnlist[i];
					$("modem_dialnum").value = daillist[i];
					$("modem_user").value = userlist[i];
					$("modem_pass").value = passlist[i];
				}
			}
		}
	}
	else {
		var ISPlist = $("modem_isp").value;
		if((ISPlist == isp) && (apn != "")){
			$("modem_apn").value = apn;
		}
		else{
			for(var i = 0; i < isplist.length; i++){
				if(isplist[i] == ISPlist){
					$("modem_apn").value = apnlist[i];
				}
			}
		}
	}
}

function applyRule(){
	showLoading(); 

	document.form.action_mode.value = " Apply ";
	document.form.current_page.value = "/Advanced_Modem_others.asp";
	document.form.next_page.value = "";

	document.form.submit();
}

function done_validating(action){
	refreshpage();
}
</script>
<style>
    .nav-tabs > li > a {
          padding-right: 6px;
          padding-left: 6px;
    }
</style>
</head>

<body onload="initial();" onunLoad="disable_auto_hint(21, 7);return unload_body();">

<div class="wrapper">
    <div class="container-fluid" style="padding-right: 0px">
        <div class="row-fluid">
            <div class="span3"><center><div id="logo"></div></center></div>
            <div class="span9" >
                <div id="TopBanner"></div>
            </div>
        </div>
    </div>

    <div id="Loading" class="popup_bg"></div>

    <iframe name="hidden_frame" id="hidden_frame" src="" width="0" height="0" frameborder="0"></iframe>

    <form method="post" name="form" id="ruleForm" action="/start_apply.htm" target="hidden_frame">
    <input type="hidden" name="productid" value="<% nvram_get_f("general.log", "productid"); %>">
    <input type="hidden" name="current_page" value="Advanced_Modem_others.asp">
    <input type="hidden" name="next_page" value="">
    <input type="hidden" name="next_host" value="">
    <input type="hidden" name="sid_list" value="General;Layer3Forwarding;">
    <input type="hidden" name="group_id" value="">
    <input type="hidden" name="modified" value="0">
    <input type="hidden" name="action_mode" value="">
    <input type="hidden" name="action_script" value="">
    <input type="hidden" name="preferred_lang" id="preferred_lang" value="<% nvram_get_x("LANGUAGE", "preferred_lang"); %>">
    <input type="hidden" name="firmver" value="<% nvram_get_x("", "firmver"); %>">

    <div class="container-fluid">
        <div class="row-fluid">
            <div class="span3">
                <!--Sidebar content-->
                <!--=====Beginning of Main Menu=====-->
                <div class="well sidebar-nav side_nav" style="padding: 0px;">
                    <ul id="mainMenu" class="clearfix"></ul>
                    <ul class="clearfix">
                        <li>
                            <div id="subMenu" class="accordion"></div>
                        </li>
                    </ul>
                </div>
            </div>

            <div class="span9">
                <!--Body content-->
                <div class="row-fluid">
                    <div class="span12">
                        <div class="box well grad_colour_dark_blue">
                            <h2 class="box_head round_top"><#menu5_4#> - <#menu5_4_4#></h2>
                            <div class="round_bottom">
                                <div class="row-fluid">
                                    <div id="tabMenu" class="submenuBlock"></div>
                                    <div class="alert alert-info" style="margin: 10px;"><#HSDPAConfig_hsdpa_mode_itemdesc#></div>

                                    <table width="100%" cellpadding="4" cellspacing="0" class="table">
                                        <tr>
                                            <th width="50%" style="border-top: 0 none;">
                                                <#HSDPAConfig_hsdpa_enable_itemname#>
                                            </th>
                                            <td style="border-top: 0 none;">
                                                <div class="main_itoggle">
                                                    <div id="modem_rule_on_of">
                                                        <input type="checkbox" id="modem_rule_fake" <% nvram_match_x("General", "modem_rule", "1", "value=1 checked"); %><% nvram_match_x("General", "modem_rule", "0", "value=0"); %>>
                                                    </div>
                                                </div>

                                                <div style="position: absolute; margin-left: -10000px;">
                                                    <input type="radio" value="1" name="modem_rule" id="modem_rule_1" class="input" <% nvram_match_x("General", "modem_rule", "1", "checked"); %>/><#checkbox_Yes#>
                                                    <input type="radio" value="0" name="modem_rule" id="modem_rule_0" class="input" <% nvram_match_x("General", "modem_rule", "0", "checked"); %>/><#checkbox_No#>
                                                </div>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_type">
                                            <th>
                                                <#ModemType#>
                                            </th>
                                            <td>
                                                <select name="modem_type" class="input" onchange="switch_modem_type();">
                                                    <option value="0" <% nvram_match_x("General", "modem_type", "0", "selected"); %>>WCDMA (UMTS)</option>
                                                    <option value="1" <% nvram_match_x("General", "modem_type", "1", "selected"); %>>CDMA2000 (EVDO)</option>
                                                    <option value="2" <% nvram_match_x("General", "modem_type", "2", "selected"); %>>TD-SCDMA</option>
                                                    <option value="3" <% nvram_match_x("General", "modem_type", "3", "selected"); %>>NDIS/RNDIS (LTE and other)</option>
                                                </select>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_country">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,9);"><#HSDPAConfig_Country_itemname#></a></th>
                                            <td>
                                                <select name="modem_country" id="isp_countrys" class="input" onchange="gen_list(document.form.modem_type.value);show_APN_list();"></select>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_isp">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,8);"><#HSDPAConfig_ISP_itemname#></a></th>
                                            <td>
                                                <select name="modem_isp" id="modem_isp" class="input" onchange="show_APN_list()"></select>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_1">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,3);"><#HSDPAConfig_private_apn_itemname#></a></th>
                                            <td>
                                                <input id="modem_apn" name="modem_apn" maxlength="32" class="input" type="text" value=""/>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_2">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,2);"><#HSDPAConfig_PIN_itemname#></a></th>
                                            <td>
                                                <div class="input-append">
                                                    <input id="wan_3g_pin" name="wan_3g_pin" class="input" type="password" maxlength="4" size="32" style="width: 175px;" value="<% nvram_get_x("", "wan_3g_pin"); %>" onkeypress="return is_number(this)"/>
                                                    <button style="margin-left: -5px;" class="btn" type="button" onclick="passwordShowHide('wan_3g_pin')"><i class="icon-eye-close"></i></button>
                                                </div>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_3">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,10);"><#HSDPAConfig_DialNum_itemname#></a></th>
                                            <td>
                                                <input id="modem_dialnum" name="modem_dialnum" class="input" type="text" value=""/>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_4">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,11);"><#HSDPAConfig_Username_itemname#></a></th>
                                            <td>
                                                <input id="modem_user" name="modem_user" class="input" type="text" value="<% nvram_get_x("", "modem_user"); %>"/>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_5">
                                            <th><a class="help_tooltip"  href="javascript:void(0);" onmouseover="openTooltip(this,21,12);"><#AiDisk_Password#></a></th>
                                            <td>
                                                <div class="input-append">
                                                    <input type="password" name="modem_pass" id="modem_pass" maxlength="32" size="32" style="width: 175px;" value="<% nvram_get_x("", "modem_pass"); %>">
                                                    <button style="margin-left: -5px;" class="btn" type="button" onclick="passwordShowHide('modem_pass')"><i class="icon-eye-close"></i></button>
                                                </div>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_6">
                                            <th><#COM_User_AT#></th>
                                            <td>
                                                <input name="modem_cmd" class="input" type="text" maxlength="40" value="<% nvram_get_x("", "modem_cmd"); %>"/>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_7">
                                            <th><#ModemZCD#></th>
                                            <td>
                                                <select name="modem_zcd" class="input">
                                                    <option value="0" <% nvram_match_x("General", "modem_zcd", "0", "selected"); %>>usb-modeswitch</option>
                                                    <option value="1" <% nvram_match_x("General", "modem_zcd", "1", "selected"); %>>legacy eject</option>
                                                </select>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_8">
                                            <th><#COM_Port_Node#></th>
                                            <td>
                                                <select name="modem_node" class="input">
                                                    <option value="0" <% nvram_match_x("General", "modem_node", "0", "selected"); %>>Auto</option>
                                                    <option value="1" <% nvram_match_x("General", "modem_node", "1", "selected"); %>>ttyUSB0/ttyACM0</option>
                                                    <option value="2" <% nvram_match_x("General", "modem_node", "2", "selected"); %>>ttyUSB1/ttyACM1</option>
                                                    <option value="3" <% nvram_match_x("General", "modem_node", "3", "selected"); %>>ttyUSB2</option>
                                                    <option value="4" <% nvram_match_x("General", "modem_node", "4", "selected"); %>>ttyUSB3</option>
                                                    <option value="5" <% nvram_match_x("General", "modem_node", "5", "selected"); %>>ttyUSB4</option>
                                                    <option value="6" <% nvram_match_x("General", "modem_node", "6", "selected"); %>>ttyUSB5</option>
                                                    <option value="7" <% nvram_match_x("General", "modem_node", "7", "selected"); %>>ttyUSB6</option>
                                                    <option value="8" <% nvram_match_x("General", "modem_node", "8", "selected"); %>>ttyUSB7</option>
                                                </select>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_ras_9">
                                            <th>NDIS MTU</th>
                                            <td>
                                                <input name="modem_mtu" class="input" type="text" maxlength="4" value="<% nvram_get_x("", "modem_mtu"); %>" onkeypress="return is_number(this)"/>
                                            </td>
                                        </tr>
                                        <tr id="row_modem_arun">
                                            <th><#ModemARun#></th>
                                            <td>
                                                <select name="modem_arun" class="input">
                                                    <option value="0" <% nvram_match_x("General", "modem_arun", "0", "selected"); %>><#checkbox_No#></option>
                                                    <option value="1" <% nvram_match_x("General", "modem_arun", "1", "selected"); %>><#checkbox_Yes#></option>
                                                    <option value="2" <% nvram_match_x("General", "modem_arun", "2", "selected"); %>><#ModemARunItem2#></option>
                                                </select>
                                            </td>
                                        </tr>
                                        <tr>
                                            <td colspan="2">
                                                <center><input type="button" class="btn btn-primary" style="width: 219px" value="<#CTL_apply#>" onclick="applyRule();"></center>
                                            </td>
                                        </tr>
                                    </table>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    </form>

    <!--==============Beginning of hint content=============-->
    <div id="help_td" style="position: absolute; margin-left: -10000px" valign="top">
        <form name="hint_form"></form>
        <div id="helpicon" onClick="openHint(0,0);"><img src="images/help.gif" /></div>

        <div id="hintofPM" style="display:none;">
            <table width="100%" cellpadding="0" cellspacing="1" class="Help" bgcolor="#999999">
            <thead>
                <tr>
                    <td>
                        <div id="helpname" class="AiHintTitle"></div>
                        <a href="javascript:;" onclick="closeHint()" ><img src="images/button-close.gif" class="closebutton" /></a>
                    </td>
                </tr>
            </thead>

                <tr>
                    <td valign="top" >
                        <div class="hint_body2" id="hint_body"></div>
                        <iframe id="statusframe" name="statusframe" class="statusframe" src="" frameborder="0"></iframe>
                    </td>
                </tr>
            </table>
        </div>
    </div>
    <!--==============Ending of hint content=============-->

    <div id="footer"></div>
</div>
</body>
</html>
