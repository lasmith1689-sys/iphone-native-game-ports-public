"""Finite source guardrails, NOT UIKit execution or a general ObjC proof."""
from pathlib import Path
import unittest

HOME=Path(__file__).resolve().parents[1]
class SourceContract(unittest.TestCase):
    def source(self):
        p=HOME/'source/NGPRetainedGameControllerV1.m'
        self.assertTrue(p.is_file(), 'actual retained UIKit caller has not been implemented')
        return p.read_text(encoding='utf-8')
    def test_real_adapter_order(self):
        s=self.source()
        # The ordered worker body, not declarations, owns these sole calls.
        body=s.split('- (void)runEngine {',1)[1]
        calls=['ngp_app_engine_session_create_v1(', 'ngp_app_engine_prepare_v1(',
               'rt_gl_bind_native_v1(', 'ngp_app_engine_attach_v1(',
               'ngp_power_worker_prepare_v1(', 'ngp_power_worker_bind_checkpoint_v1(',
               'requestActivationForPreparedWorker:', '[self awaitActualPower]',
               'ngp_app_engine_common_v1(', 'ngp_app_engine_frame_v1(']
        positions=[body.index(c) for c in calls]
        self.assertEqual(positions,sorted(positions))
        for c in calls:
            self.assertEqual(s.count(c),1, 'one static callsite, no fallback/retry: '+c)
    def test_no_guest_authority_or_unverified_join(self):
        s=self.source()
        for forbidden in ['pthread_join(', 'ngp_power_join_worker_v1(',
                          'ngp_power_worker_unbind_checkpoint_v1(',
                          'finishAfterWorkerJoin:', 'rt_core_run_target(',
                          'CodNativeInputV1 input=', 'rt_mem(', 'free(',
                          'dispatch_sync(dispatch_get_main_queue()', 'waitUntilFinished:YES']:
            self.assertNotIn(forbidden,s)
    def test_lifecycle_serializes_before_pause(self):
        s=self.source()
        self.assertIn('atomic_fetch_or_explicit(&_lane, NGP_LANE_CANCEL',s)
        self.assertIn('atomic_compare_exchange_strong_explicit(&_lane',s)
        self.assertIn('atomic_fetch_and_explicit(&_lane',s)
        body=s.split('- (void)pauseIfIdleOnMain {',1)[1].split('\n}',1)[0]
        self.assertLess(body.index('NGP_LANE_BUSY'),body.index('cod_gl_host_v1_pause('))
    def test_immutable_owned_inputs_and_copied_observation(self):
        s=self.source()
        self.assertIn('initWithBytes:base length:count',s)
        self.assertIn('dataWithBytes:&observation length:sizeof(observation)',s)
        self.assertIn('calloc(1, sizeof(*_frame))',s)
        self.assertIn('calloc(1, sizeof(*_snapshot))',s)
        self.assertIn('[self parkActualThread]',s)
    def test_command_count_is_bound_to_owned_copy(self):
        s=self.source()
        self.assertFalse('command.length' in s,'command count must come from retained copy')
        self.assertIn('_commandCount=(uint32_t)commandCopy.length',s)
        self.assertIn('i<commandCopy.length',s)
if __name__=='__main__':unittest.main()
