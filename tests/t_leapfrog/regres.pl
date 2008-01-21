#
#  $Id: regres.pl,v 3.19 2008-01-21 21:45:11 edwards Exp $
#
#  This is the portion of a script this is included recursively
#

#
# Each test has a name, input file name, output file name,
# and the good output that is tested against.
#
@regres_list = 
    (
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.unprec_clover.ini.xml" , 
	 log         => "t_leapfrog.unprec_clover.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.unprec_clover.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.unprec_clover.log.xml" ,
     },  
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.ini.xml" , 
	 log         => "t_leapfrog.prec_2flav_clover.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.sfnonpt.ini.xml" , 
	 log         => "t_leapfrog.prec_2flav_clover.sfnonpt.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.sfnonpt.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover.sfnonpt.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.lw.sfnonpt.ini.xml" , 
	 log         => "t_leapfrog.lw.sfnonpt.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.lw.sfnonpt.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.lw.sfnonpt.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover_ee_oo.ini.xml" , 
	 log         => "t_leapfrog.prec_2flav_clover.ee_oo_candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover_ee_oo.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_2flav_clover_ee_oo.log.xml" ,
     },


     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.ini.xml" , 
	 log         => "t_leapfrog.prec_wilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.bicgstab.ini.xml" , 
	 log         => "t_leapfrog.prec_wilson.bicgstab.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.bicgstab.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_wilson.bicgstab.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_parwilson.ini.xml" , 
	 log         => "t_leapfrog.prec_parwilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_parwilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_parwilson.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.parwilson_pqp_integrator_mts.ini.xml" , 
	 log         => "t_leapfrog.parwilson_pqp_integrator_mts.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.parwilson_pqp_integrator_mts.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.parwilson_pqp_integrator_mts.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_integrator_mts.ini.xml" , 
	 log         => "t_leapfrog.parwilson_minimal_norm_2nd_order_integrator_mts.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_integrator_mts.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_integrator_mts.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_qpq_integrator_mts.ini.xml" , 
	 log         => "t_leapfrog.parwilson_minimal_norm_2nd_order_qpq_integrator_mts.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_qpq_integrator_mts.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.parwilson_minimal_norm_2nd_order_qpq_integrator_mts.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_dwf.ini.xml" , 
	 log         => "t_leapfrog.prec_dwf.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_dwf.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_dwf.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_contfrac.ini.xml" , 
	 log         => "t_leapfrog.prec_contfrac.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_contfrac.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_contfrac.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.unprec_dwf.ini.xml" , 
	 log         => "t_leapfrog.unprec_dwf.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.unprec_dwf.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.unprec_dwf.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.unprec_wilson.ini.xml" , 
	 log         => "t_leapfrog.unprec_wilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.unprec_wilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.unprec_wilson.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch_wilson.ini.xml" , 
	 log         => "t_leapfrog.unprec_hasenbusch_wilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch_wilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch_wilson.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch_wilson.ini.xml" , 
	 log         => "t_leapfrog.prec_hasenbusch_wilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch_wilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch_wilson.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_1flav_wilson.ini.xml" , 
	 log         => "t_leapfrog.prec_1flav_wilson.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_1flav_wilson.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_1flav_wilson.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact.ini.xml" , 
	 log         => "t_leapfrog.rect_gaugeact.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_1.ini.xml" , 
	 log         => "t_leapfrog.rect_gaugeact_1.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_1.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_1.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_c1t0.ini.xml" , 
	 log         => "t_leapfrog.rect_gaugeact_c1t0.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_c1t0.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_c1t0.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_omit2linkT.ini.xml" , 
	 log         => "t_leapfrog.rect_gaugeact_omit2linkT.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_omit2linkT.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_omit2linkT.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_aniso.ini.xml" , 
	 log         => "t_leapfrog.rect_gaugeact_aniso.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_aniso.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.rect_gaugeact_aniso.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.two_plaq_spatial_gaugeact.ini.xml" , 
	 log         => "t_leapfrog.two_plaq_spatial_gaugeact.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.two_plaq_spatial_gaugeact.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.two_plaq_spatial_gaugeact.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.aniso_spectrum.ini.xml" , 
	 log         => "t_leapfrog.aniso_spectrum.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.aniso_spectrum.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.aniso_spectrum.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.fixed_X_action_stout.ini.xml" , 
	 log         => "t_leapfrog.fixed_X_action_stout.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.fixed_X_action_stout.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.fixed_X_action_stout.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_clover_stout.ini.xml" , 
	 log         => "t_leapfrog.prec_clover_stout.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_clover_stout.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_clover_stout.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.ini.xml" , 
	 log         => "t_leapfrog.prec_slrc.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.sfnonpt.ini.xml" , 
	 log         => "t_leapfrog.prec_slrc.sfnonpt.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.sfnonpt.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_slrc.sfnonpt.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch5d_dwf.ini.xml" , 
	 log         => "t_leapfrog.prec_hasenbusch5d_dwf.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch5d_dwf.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.prec_hasenbusch5d_dwf.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch5d_dwf.ini.xml" , 
	 log         => "t_leapfrog.unprec_hasenbusch5d_dwf.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch5d_dwf.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.unprec_hasenbusch5d_dwf.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact.ini.xml" , 
	 log         => "t_leapfrog.wilson_gaugeact.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_st.ini.xml" , 
	 log         => "t_leapfrog.wilson_gaugeact_st.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_st.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_st.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_aniso_mom.ini.xml",
	 log         => "t_leapfrog.wilson_gaugeact_aniso_mom.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_aniso_mom.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.wilson_gaugeact_aniso_mom.log.xml" ,
     },

     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial_plus_temporal.ini.xml" , 
	 log         => "t_leapfrog.aniso_sym_spatial_plus_temporal.log.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial_plus_temporal.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial_plus_temporal.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial.ini.xml" , 
	 log         => "t_leapfrog.aniso_sym_spatial.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_spatial.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_temporal.ini.xml",
	 log         => "t_leapfrog.aniso_sym_temporal.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_temporal.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.aniso_sym_temporal.log.xml" ,
     },
     {
	 exec_path   => "$top_builddir/mainprogs/tests" , 
	 execute     => "t_leapfrog" , 
	 input       => "$test_dir/t_leapfrog/t_leapfrog.2+2_gaugeact.ini.xml",
	 log         => "t_leapfrog.2+2_gaugeact.candidate.xml",
	 metric      => "$test_dir/t_leapfrog/t_leapfrog.2+2_gaugeact.metric.xml" ,
	 controlfile => "$test_dir/t_leapfrog/t_leapfrog.2+2_gaugeact.log.xml" ,
     }
     );
