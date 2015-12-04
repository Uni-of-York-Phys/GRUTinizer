#include "TWriteLoop.h"

#include "TFile.h"

#include <chrono>
#include <thread>

//#include "TPreserveGDirectory.h"

TWriteLoop* TWriteLoop::Get(std::string name, std::string output_filename){
  if(name.length()==0){
    name = "write_loop";
  }

  StoppableThread* thread = StoppableThread::Get(name);
  if(!thread){
    if(output_filename.length()==0){
      output_filename = "temp.root";
    }
    thread = new TWriteLoop(name,output_filename);
  }

  return dynamic_cast<TWriteLoop*>(thread);
}

TWriteLoop::TWriteLoop(std::string name, std::string output_filename)
  : StoppableThread(name),
    in_learning_phase(true), learning_phase_length(1000),
    event_tree_size(0), learning_phase_size(0) {
  //TPreserveGDirectory preserve;
  output_file = new TFile(output_filename.c_str(),"RECREATE");
  event_tree = new TTree("EventTree","EventTree");
  //scaler_tree = new TTree("ScalerTree","ScalerTree");
}

TWriteLoop::~TWriteLoop() {
  std::cout << __PRETTY_FUNCTION__ << 0 << std::endl;
  if(in_learning_phase){
    EndLearningPhase();
  }
  std::cout << __PRETTY_FUNCTION__ << 1 << std::endl;
  
  for(auto& elem : det_map){
    delete *elem.second;
    delete elem.second;
  }
  std::cout << __PRETTY_FUNCTION__ << 2 << std::endl;

  event_tree->Write();
  std::cout << __PRETTY_FUNCTION__ << 3 << std::endl;
  output_file->Close();
  output_file->Delete();
  std::cout << __PRETTY_FUNCTION__ << 4 << std::endl;
}

void TWriteLoop::Connect(TUnpackingLoop* input_queue){
  if(input_queue){
    std::lock_guard<std::mutex> lock(input_queue_mutex);
    input_queues.push_back(input_queue);
  }
}

bool TWriteLoop::Iteration() {
  TUnpackedEvent* event = NULL;

  bool handled_event = false;
  bool living_parent = false;

  {
    std::lock_guard<std::mutex> lock(input_queue_mutex);
    for(auto& queue : input_queues){
      int size = queue->Pop(event,0);
      if(size >= 0) {
        HandleEvent(event);
        handled_event = true;
        living_parent = true;
      } else if (queue->IsRunning()){
        living_parent = true;
      }
    }
  }

  if(!handled_event){
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return living_parent || (input_queues.size()==0);
}

void TWriteLoop::Write() {
  EndLearningPhase();
  //TPreserveGDirectory preserve;
  output_file->cd();
  event_tree->Write();
}

void TWriteLoop::HandleEvent(TUnpackedEvent* event) {
  if(in_learning_phase) {
    LearningPhase(event);
    learning_phase_size++;
    if(learning_queue.size() > learning_phase_length){
      EndLearningPhase();
    }
  } else {
    WriteEvent(event);
  }
}

void TWriteLoop::LearningPhase(TUnpackedEvent* event) {
  for(auto det : event->GetDetectors()) {
    TClass* cls = det->IsA();
    if(!det_map.count(cls)){
      TDetector** det = new TDetector*;
      *det = (TDetector*)cls->New();
      det_map[cls] = det;
      // TODO: Place this mutex here
      event_tree->Branch(cls->GetName(), cls->GetName(), det);
    }
  }
  learning_queue.push_back(event);
}

void TWriteLoop::EndLearningPhase() {
  in_learning_phase = false;
  for(auto event : learning_queue){
    WriteEvent(event);
    learning_phase_size--;
  }
  learning_queue.clear();
}

void TWriteLoop::WriteEvent(TUnpackedEvent* event) {
  event_tree_size++;
  // Clear pointers from previous writes.
  for(auto& elem : det_map){
    *elem.second = NULL;
  }

  // Load current events
  for(auto det : event->GetDetectors()) {
    TClass* cls = det->IsA();
    *det_map.at(cls) = det;
  }

  // Fill
  event_tree->Fill();
  delete event;
}
