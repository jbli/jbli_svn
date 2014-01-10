#!/usr/bin/perl

$carbon_prefix='def_';
$full_hname=`hostname`;
if($full_hname =~ /([^.]+)\..*/){
    $short_hname = $1;
}
$traffic_path_prefix = $carbon_prefix . $short_hname . ".sys.traffic";
$packet_path_prefix = $carbon_prefix . $short_hname . ".sys.packet";
open(OUTFILE, ">>/data/tools/log/vnstat.log")  || die ("Could not open file");


$res=`vnstat -tr 5 -i eth1 |egrep 'rx|tx'`;
@line=split(/\n/, $res);
foreach $item (@line){
  @arry=split(/\s+/, $item);
  $path="";
  $package_path="";
  $val = 0;
  $traffic_path=$traffic_path_prefix.".".$arry[1];
  $packet_path=$packet_path_prefix.".".$arry[1];
  if ($arry[3] eq "kbit/s"){
    $val = sprintf("%.4f",$arry[2]/1024);
  }elsif($arry[3] eq "Mbit/s"){
    $val = $arry[2];
  }else{
    $val = 0;
  }
  $timestr=`date +"%Y-%m-%d %H:%M:%S"`;
  $traffic_str = $traffic_path . " " . $val . " " . time() . " ".$timestr;
  $packet_str = $packet_path . " " . $arry[4] . " " . time() . " ".$timestr;
  print OUTFILE $traffic_str;
  print OUTFILE $packet_str;
}
close(OUTFILE);
